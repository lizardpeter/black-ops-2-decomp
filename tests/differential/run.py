#!/usr/bin/env python3
"""Differential tests: every exported reconstructed function against the original, on the same inputs.

    python3 tests/differential/run.py [--build <id>] [--check-records] [-k <name>]

Builds the comparison for one build (default: the reference build, CoDMPServer_PC): its original exe and
build/<build>/recon.dll (tools/build_recon.py), mapped into one emulator (harness.py). Cases live in
tests/differential/test_*.py as CASES, keyed by a function's scoped plain name (`ClampChar`,
`create@bdPlatformSocket`), so one case serves the x86 servers and the x64 Store build alike:

    {"name": "ClampChar", "ret": "char", "inputs": [[-129], [0], ...]}
    {"name": "Com_HashKey", "ret": "int", "inputs": [{"args": [b"abc\\0", 3], "setup": fn(writer)}, ...],
     "fakes": {"Name": fn(mu)}, "observe": {"Name": "is"}, "builds": ["CoDMPServer_PC"]}

A case passes when, for every input, both runs return the same value (floats compared bit for bit), leave the same
bytes in every buffer passed by pointer, write the same bytes to the images' globals, and make the same observed
calls. An input on which the original itself faults is skipped; a case with no usable input is untestable.

--check-records fails if records/functions.tsv claims `matches` (or `integrated`) for a function of this build that
did not pass. Results go to build/<build>/test-results.tsv.
"""
import importlib.util
import math
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(Path(__file__).parent))
import symbols  # noqa: E402
from harness import Image, Machine, original_exe  # noqa: E402

MATCHED = {"matches", "integrated"}


def same(a, b):
    if isinstance(a, float) and isinstance(b, float):
        if math.isnan(a) and math.isnan(b):
            return True
        return struct.pack("<d", a) == struct.pack("<d", b)
    return a == b


def load_cases():
    cases = {}
    for path in sorted(Path(__file__).parent.glob("test_*.py")):
        spec = importlib.util.spec_from_file_location(path.stem, path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        for case in getattr(mod, "CASES", []):
            if case["name"] in cases:
                sys.exit(f"{path.name}: a second case for {case['name']}")
            cases[case["name"]] = case
    return cases


class Canon:
    """Make addresses from the two images comparable: a reconstructed function -> the original it stands for, a
    hybrid-link thunk -> its target (x86 `push va; ret`, x64 `jmp [rip+slot]`), a string literal -> its text."""

    def __init__(self, m, recon, recon_to_orig):
        self.m, self.recon, self.recon_to_orig = m, recon, recon_to_orig

    def code(self, v):
        r = self.recon
        if not r.base <= v < r.base + r.size:
            return v
        if v in self.recon_to_orig:
            return self.recon_to_orig[v]
        try:
            code = bytes(self.m.mu.mem_read(v, 6))
        except Exception:
            return v
        if not r.x64 and code[0] == 0x68 and code[5] == 0xC3:
            return struct.unpack("<I", code[1:5])[0]
        if r.x64 and code[:2] == b"\xff\x25":
            return self.m._ptr(v + 6 + struct.unpack("<i", code[2:6])[0])
        return v

    def writes(self, writes):
        out, width = dict(writes), 8 if self.m.x64 else 4
        fmt = "<Q" if width == 8 else "<I"
        for a in sorted(writes):
            if a % width or any(a + i not in out for i in range(width)) or not isinstance(out[a], bytes):
                continue
            v = struct.unpack(fmt, b"".join(out[a + i] for i in range(width)))[0]
            c = self.code(v)
            if c != v:
                for i, byte in enumerate(struct.pack(fmt, c)):
                    out[a + i] = bytes([byte])
            elif any(lo <= v < hi for img in self.m.images for lo, hi in img.rdata):
                for i in range(1, width):
                    del out[a + i]
                out[a] = ("str", self.m.read_cstr(v))
        return out


def resolve_hooks(spec, build, m, recon):
    """{name or address: x} -> {address: x}, hooking both copies when the reconstruction defines one."""
    out = {}
    for key, x in (spec or {}).items():
        if isinstance(key, int):
            out[key] = x
            continue
        if "!" in key:
            out[m.import_stub(key)] = x
            continue
        hit = symbols.resolve(build, key) or next(
            ((va, k) for s, (va, k) in symbols.table(build).items() if symbols.scoped_name(s) == key), None)
        if hit:
            out[hit[0]] = x
        for e, va in recon.exports.items():
            if symbols.scoped_name(e) == key:
                out[va] = x
    return out


def main():
    build = sys.argv[sys.argv.index("--build") + 1] if "--build" in sys.argv else symbols.reference_build()
    only = sys.argv[sys.argv.index("-k") + 1] if "-k" in sys.argv else None
    out = ROOT / "build" / build
    recon = Image(out / "recon.dll")
    m = Machine(Image(original_exe(build)), recon)
    rows = symbols.read_tsv(out / "recon.symbols.tsv")
    export_name = (lambda s: s) if recon.x64 else (lambda s: s[1:] if s.startswith("_") else s)
    recon_to_orig = {recon.exports[export_name(r["symbol"])]: int(r["va"], 16) for r in rows
                     if r["va"] and export_name(r["symbol"]) in recon.exports}
    canon = Canon(m, recon, recon_to_orig)
    cases = load_cases()

    results, failed = [], 0
    for r in sorted(rows, key=lambda r: r["va"]):
        name = symbols.scoped_name(r["symbol"])
        if only and only not in name:
            continue
        case = cases.get(name)
        va, target = int(r["va"], 16), recon.exports.get(export_name(r["symbol"]))
        if case is None or (case.get("builds") and build not in case["builds"]):
            results.append([build, r["va"], r["symbol"], 0, "untested: no case"])
            print(f"untested   {r['va']} {name}")
            continue
        fakes = resolve_hooks(case.get("fakes"), build, m, recon)
        observe = resolve_hooks(case.get("observe"), build, m, recon)
        verdict, used = "pass", 0
        for inp in case["inputs"]:
            args, setup = (inp["args"], inp.get("setup")) if isinstance(inp, dict) else (inp, None)
            want = m.call(va, args, case["ret"], setup, fakes, observe)
            if isinstance(want[0], str) and want[0].startswith("fault"):
                continue  # the original itself crashes on this input: not a usable case
            got = m.call(target, args, case["ret"], setup, fakes, observe)
            got_calls = [tuple(canon.code(x) if isinstance(x, int) else x for x in c) for c in got[3]]
            used += 1
            ww, gw = canon.writes(want[2]), canon.writes(got[2])
            if not (same(want[0], got[0]) and want[1] == got[1] and ww == gw and want[3] == got_calls):
                diff = sorted(a for a in set(ww) | set(gw) if ww.get(a) != gw.get(a))[:6]
                verdict = (f"FAIL args={args!r}\n       original={want[0]!r} recon={got[0]!r} "
                           f"buffers_equal={want[1] == got[1]} writes_equal={ww == gw} calls_equal={want[3] == got_calls}"
                           + (f"\n       writes differ at {[hex(a) for a in diff]}" if diff else ""))
                break
        if verdict == "pass" and not used:
            verdict = f"untestable: the original faults on all {len(case['inputs'])} inputs"
        failed += verdict != "pass"
        results.append([build, r["va"], r["symbol"], used, verdict.split("\n")[0]])
        tag = "ok  " if verdict == "pass" else ("UNTESTABLE" if verdict.startswith("untestable") else "FAIL")
        print(f"{tag} {r['va']} {name}  ({used} cases)" + ("" if verdict == "pass" else f"\n     {verdict}"))

    (out / "test-results.tsv").write_text("build\tva\tsymbol\tcases\tresult\n" +
                                          "".join("\t".join(map(str, x)) + "\n" for x in results))
    passed = {x[1] for x in results if x[4] == "pass"}
    if "--check-records" in sys.argv:
        for rec in symbols.read_tsv(ROOT / "records" / "functions.tsv"):
            ran = {x[1] for x in results}  # with -k, only the functions that ran
            if rec["build"] == build and rec["status"] in MATCHED and rec["va"] not in passed and \
                    (not only or rec["va"] in ran):
                print(f"FAIL records claim {rec['status']} for {build}:{rec['va']} without a passing test")
                failed += 1
    print(f"{build}: {len(passed)} passed, {failed} failed, "
          f"{sum(1 for x in results if x[4].startswith('untested'))} without a case")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
