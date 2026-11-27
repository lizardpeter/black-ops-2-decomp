"""The symbol table of a build: decorated name -> (address, kind), for linking and testing the reconstruction.

- A reference build (MAP + PDB) has its own: MAP publics (kind function/data from the MAP flags) and PDB statics
  (as `t6s@<source>@<scope>@<name>`, see tools/recon/include/t6_static.h).
- Any other build (the Store build, the Steam exe) gets one from identifications:
  1. records/names.tsv: authored identifications for that build, each with its method, evidence and confidence;
  2. analysis/<build>/match/functions.tsv: the matcher's reference -> target pairs, taking the reference
     function's decorated name from the reference MAP. A reference address that carries several names (identical
     code folding) is skipped, since the target build need not have folded the same functions.
  records/names.tsv wins over the matcher when both name the same symbol.

The references are x86 and the Store build is x64, so one function's decorated name differs between them (x64 adds
`E` pointer modifiers and has one calling convention; x86 C names carry a leading underscore). A build without a MAP
therefore resolves a name the reconstruction emits through its plain name (`resolve`). The prototype itself is
checked where it can be: against the x86 reference's MAP.
"""
import re
import csv
import tomllib
from functools import lru_cache
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STATIC_PREFIX = "t6s@"
csv.field_size_limit(1 << 30)


def read_tsv(path):
    if not Path(path).exists():
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f, delimiter="\t", quoting=csv.QUOTE_NONE))


def config():
    return tomllib.loads((ROOT / "target.toml").read_text())


def build_spec(build: str) -> dict:
    return next(b for b in config()["build"] if b["id"] == build)


def architecture(build: str) -> str:
    return build_spec(build).get("architecture", "x86")


def reference_build() -> str:
    """The first reference build with a MAP: tests and the default hybrid link use it (CoDMPServer_PC)."""
    return next(b["id"] for b in config()["build"] if b.get("kind") == "reference" and "map" in b)


def retail_builds() -> list:
    return [b["id"] for b in config()["build"] if b.get("kind") == "retail"]


def has_map(build: str) -> bool:
    return (ROOT / "analysis" / build / "map" / "publics.tsv").exists()


def scoped_name(sym: str) -> str:
    """A symbol's name with its class scope and without decoration or the x86 C underscore:
    `?create@bdPlatformSocket@@SAH...` -> `create@bdPlatformSocket`, `?ClampChar@@YADH@Z` -> `ClampChar`,
    `_memset` -> `memset`. Identical for one function's x86 and x64 names."""
    if sym.startswith("?"):
        return sym[1:].split("@@", 1)[0]
    if sym.startswith(STATIC_PREFIX):
        return sym
    return sym[1:] if sym.startswith("_") else sym


def resolve(build: str, sym: str):
    """(va, kind) of a symbol in a build, or None. Exact first; in a build without a MAP, through its scoped plain
    name when exactly one address carries it."""
    t = table(build)
    if sym in t:
        return t[sym]
    if has_map(build):
        return None
    hits = {v for k, v in t.items() if scoped_name(k) == scoped_name(sym) and same_overload(k, sym)}
    return hits.pop() if len(hits) == 1 else None


def _arguments(decorated: str) -> str:
    """A function's decorated signature without the 64-bit pointer marks (x64 'PEA' is x86 'PA', and so on), so a
    name recorded from either build compares with the reconstruction's."""
    sig = decorated.split("@@Y", 1)[1]
    return re.sub(r"(?<=[PQAR])E(?=[ABCD])", "", sig)


def same_overload(a: str, b: str) -> bool:
    """Two names of one function, not two overloads: true unless both are decorated free functions (`@@Y`) whose
    signatures differ."""
    if not (a.startswith("?") and b.startswith("?") and "@@Y" in a and "@@Y" in b):
        return True
    return _arguments(a) == _arguments(b)


# match.py methods trusted without review for linking: each rests on the function's own content (its strings,
# registrations, constants, named globals). Call-graph propagation (callee, caller, order, votes) can't tell an
# inlined callee from the one left in its place in a build with link-time code generation, so its pairs reach the
# link only through records/names.tsv. Neither do anchored pairs whose function has a single caller the target could
# be instead, with the function inlined into it (methods "<method>-or-caller" and "<method>-via-callee",
# tools/match/match.py inlined_into_caller): CG_AirburstLazeCancelledByMovement's globals anchored the Store's
# CL_UpdateAirburstMarkDistance, which inlines it.
ANCHORED = {"string", "named", "constant", "global"}


@lru_cache(maxsize=None)
def table(build: str) -> dict:
    """{symbol: (va, kind)} with kind 'function' or 'data'."""
    out = {}
    if has_map(build):
        for r in read_tsv(ROOT / "analysis" / build / "map" / "publics.tsv"):
            if r["obj"] == "<absolute>":
                continue
            out.setdefault(r["name"], (int(r["va"], 16), "function" if "f" in r["flags"] else "data"))
        for r in read_tsv(ROOT / "analysis" / build / "pdb" / "statics.tsv"):
            if r["va"] and r["source"]:
                out.setdefault(f"{STATIC_PREFIX}{r['source']}@{r['scope']}@{r['name']}", (int(r["va"], 16), "data"))
        return out

    matched = read_tsv(ROOT / "analysis" / build / "match" / "functions.tsv")
    for ref in sorted({r["ref_build"] for r in matched}):
        names_at = {}
        for r in read_tsv(ROOT / "analysis" / ref / "map" / "publics.tsv"):
            if "f" in r["flags"]:
                names_at.setdefault(r["va"], []).append(r["name"])
        targets = {}
        for r in matched:
            if r["ref_build"] != ref or len(names_at.get(r["ref_va"], [])) != 1:
                continue
            if r["method"] not in ANCHORED:
                continue  # a propagated pair is used once reviewed (records/names.tsv): inlining misleads it
            targets.setdefault(names_at[r["ref_va"]][0], set()).add(int(r["target_va"], 16))
        for name, vas in targets.items():
            if len(vas) == 1:
                out[name] = (vas.pop(), "function")
    # Store MP renderer aliases are ABI boundary names, not claimed original
    # symbols. They are independently audited against the exact PE and replay.
    authored = read_tsv(ROOT / "records" / "names.tsv")
    if build == "t6mp_msstore":
        authored += read_tsv(ROOT / "records" / "store-renderer-bindings.tsv")
    for r in authored:
        if r["build"] == build:
            out[r["symbol"] or r["name"]] = (int(r["va"], 16), r["kind"])
    return out


@lru_cache(maxsize=None)
def conventions(build: str) -> dict:
    """{va: [location per argument]} for the build's functions with a custom calling convention
    (records/conventions.tsv): link-time code generation passes some arguments of functions it sees all callers of in
    registers. A location is "stack" or a register ("eax", "ecx", "edx", "ebx", "esi", "edi", "xmm0".."xmm3")."""
    path = ROOT / "records" / "conventions.tsv"
    if not path.exists():
        return {}
    return {int(r["va"], 16): [a.strip() for a in r["args"].split(",")] if r["args"] else []
            for r in read_tsv(path) if r["build"] == build}
