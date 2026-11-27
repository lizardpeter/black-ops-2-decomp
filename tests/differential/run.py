#!/usr/bin/env python3
"""Differential tests: every exported reconstructed function against the original, on the same inputs.

    python3 tests/differential/run.py [--build <id>] [--enhanced] [--check-records] [-k <name>]

Builds the comparison for one build (default: the reference build, CoDMPServer_PC): its original exe and
build/<build>/recon.dll (tools/build_recon.py), mapped into one emulator (harness.py). Cases live in
tests/differential/test_*.py as CASES, keyed by a function's scoped plain name (`ClampChar`,
`create@bdPlatformSocket`), so one case serves the x86 servers and the x64 Store build alike:

    {"name": "ClampChar", "ret": "char", "inputs": [[-129], [0], ...]}
    {"name": "Com_HashKey", "ret": "int", "inputs": [{"args": [b"abc\\0", 3], "setup": fn(writer)}, ...],
     "fakes": {"Name": fn(mu)}, "observe": {"Name": "is"}, "builds": ["CoDMPServer_PC"],
     "imports": {"winmm.dll!mixerOpen": (fn(machine, mu), nargs)}}

An original with a custom calling convention (records/conventions.tsv) is called with it; the reconstruction always
takes the standard one.

A case whose `fakes` name a function the build doesn't identify yet is reported as blocked, not run. A case passes
when, for every input, both runs return the same value (floats compared bit for bit), leave the same
bytes in every buffer passed by pointer, write the same bytes to the images' globals, and make the same observed
calls. An input on which the original itself faults is skipped; a case with no usable input is untestable.

--enhanced tests build/<build>/enhanced/recon.dll instead: every function must still match the original except the
ones enhanced/ overrides, which tests/enhanced/ checks against their patches (records/patches.tsv).

--check-records fails if records/functions.tsv claims `matches` (or `integrated`) for a function of this build that
did not pass here or in the native replay tests (build/<build>/replay-results.tsv, tests/replay/run.py). Results go to build/<build>/test-results.tsv.
"""
import importlib.util
import math
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(Path(__file__).parent))
import symbols  # noqa: E402
from harness import Image, Machine, original_exe  # noqa: E402

MATCHED = {"matches", "integrated"}
ASSERTS = True


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
        if not isinstance(v, int) or not r.base <= v < r.base + r.size:  # e.g. com.py's ("com", ...) entries
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


def resolve_hooks(spec, build, m, recon, missing=None):
    """{name or address: x} -> {address: x}, hooking both copies when the reconstruction defines one. Names the
    original build doesn't identify go into `missing`."""
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
        elif missing is not None:
            missing.append(key)
        for e, va in recon.exports.items():
            # a decorated key hooks the reconstruction too; a plain key is matched as written as well, since
            # scoped_name reads a leading underscore as C decoration (_Dvar_RegisterFloat is the C++ name itself)
            if symbols.scoped_name(e) in (symbols.scoped_name(key), key):
                out[va] = x
    return out


def trap_at(fault):
    """The faulting instruction is a hybrid-link trap (ud2) for a symbol the build doesn't identify yet."""
    m = re.search(r"at ([0-9A-Fa-f]+)", fault)
    if not m:
        return False
    try:
        return bytes(MACHINE.mu.mem_read(int(m.group(1), 16), 2)) == b"\x0f\x0b"
    except Exception:
        return False


def recon_convention(symbol, nargs):
    """The reconstruction's own convention: standard, except a __thiscall member (this in ECX)."""
    if re.search(r"@@[QAIEMU][AB]E", symbol):
        return ["ecx"] + ["stack"] * (nargs - 1)
    return None


def without_asserts(spec):
    """Hooks on Assert_MyHandler mean nothing in a build that compiled its asserts out (target.toml `asserts`)."""
    if ASSERTS or not spec:
        return spec
    return {k: v for k, v in spec.items() if k != "Assert_MyHandler"}


def first_diff(a, b):
    """The index of the first call where two call lists differ (the shorter one's length if one is a prefix)."""
    return next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))


def main():
    build = sys.argv[sys.argv.index("--build") + 1] if "--build" in sys.argv else symbols.reference_build()
    only = sys.argv[sys.argv.index("-k") + 1] if "-k" in sys.argv else None
    global ASSERTS
    ASSERTS = bool(symbols.build_spec(build).get("asserts"))
    enhanced = "--enhanced" in sys.argv
    out = ROOT / "build" / build / ("enhanced" if enhanced else "")
    recon = Image(out / "recon.dll")
    m = Machine(Image(original_exe(build)), recon)
    global MACHINE
    MACHINE = m
    rows = symbols.read_tsv(out / "recon.symbols.tsv")
    # what the reconstruction calls in the original: a fake for a function it doesn't call in this build (code
    # compiled out for it, e.g. the server's warnings in the Store revision) can't block a case
    referenced = {symbols.scoped_name(r["symbol"]) for r in symbols.read_tsv(out / "hybrid.tsv")}
    export_name = (lambda s: s) if recon.x64 else (lambda s: s[1:] if s.startswith("_") else s)
    recon_to_orig = {recon.exports[export_name(r["symbol"])]: int(r["va"], 16) for r in rows
                     if r["va"] and export_name(r["symbol"]) in recon.exports}
    crt = [symbols.resolve(build, n) for n in ("__getptd", "__getptd_noexit", "___ptmbcinfo", "___ptlocinfo")]
    if not recon.x64 and all(crt):
        m.crt_thread_data([crt[0][0], crt[1][0]], crt[2][0], crt[3][0])
    canon = Canon(m, recon, recon_to_orig)
    conventions = symbols.conventions(build)
    cases = load_cases()

    results, failed = [], 0
    for r in sorted(rows, key=lambda r: r["va"]):
        name = symbols.scoped_name(r["symbol"])
        if only and only not in name and only not in r["source"]:
            continue
        if r["source"].startswith("enhanced/"):
            results.append([build, r["va"], r["symbol"], 0, "patched: tests/enhanced"])
            print(f"patched    {r['va']} {name}  ({r['source']}: tests/enhanced)")
            continue
        case = cases.get(r["symbol"]) or cases.get(name)  # a decorated case name tells overloads apart
        if case and build in case.get("by_build", {}):
            # per-build overrides: what a build inlined can't be faked or observed there
            case = dict(case, **case["by_build"][build])
        va, target = int(r["va"], 16), recon.exports.get(export_name(r["symbol"]))
        if case is None or (case.get("builds") and build not in case["builds"]):
            results.append([build, r["va"], r["symbol"], 0, "untested: no case"])
            print(f"untested   {r['va']} {name}")
            continue
        missing = []
        fakes = resolve_hooks(without_asserts(case.get("fakes")), build, m, recon, missing)
        missing = [k for k in missing if symbols.scoped_name(k) in referenced or k in referenced]
        observe = resolve_hooks(without_asserts(case.get("observe")), build, m, recon)
        if missing:
            # the case replaces a function this build doesn't identify yet: the original would run the real one
            results.append([build, r["va"], r["symbol"], 0, f"blocked: needs {', '.join(missing)} identified"])
            print(f"blocked    {r['va']} {name}  (fakes {', '.join(missing)}: not identified in {build} yet)")
            continue
        for name_, (fn, nargs) in (case.get("imports") or {}).items():
            m.on_import(name_, (lambda f: lambda mu: f(m, mu))(fn), nargs)
        verdict, used = "pass", 0
        for index, inp in enumerate(case["inputs"]):
            if isinstance(inp, dict) and inp.get("builds") and build not in inp["builds"]:
                continue  # an input one build can't be compared on (the input's comment says why)
            args, setup = (inp["args"], inp.get("setup")) if isinstance(inp, dict) else (inp, None)
            want = m.call(va, args, case["ret"], setup, fakes, observe, convention=conventions.get(va))
            if isinstance(want[0], str) and want[0].startswith("fault"):
                continue  # the original itself crashes on this input: not a usable case
            got = m.call(target, args, case["ret"], setup, fakes, observe,
                         convention=recon_convention(r["symbol"], len(args)))
            if isinstance(got[0], str) and "UC_ERR_INSN_INVALID" in got[0] and trap_at(got[0]):
                verdict = "blocked: reaches a function not identified in this build yet (a trap)"
                break
            # the hooked function, and pointer-sized arguments (q), may be reconstructed code: map them back to the
            # original's. Integers (i) are values, even when they happen to fall inside the DLL's address range.
            got_calls = [c if not isinstance(c[0], int) else (canon.code(c[0]),) + tuple(canon.code(x) if k == "q" and isinstance(x, int) else x
                                                    for k, x in zip(observe.get(c[0], ""), c[1:])) for c in got[3]]
            used += 1
            ww, gw = canon.writes(want[2]), canon.writes(got[2])
            if not (same(want[0], got[0]) and want[1] == got[1] and ww == gw and want[3] == got_calls):
                diff = sorted(a for a in set(ww) | set(gw) if ww.get(a) != gw.get(a))[:6]
                verdict = (f"FAIL input {index} args={args!r}\n       original={want[0]!r} recon={got[0]!r} "
                           f"buffers_equal={want[1] == got[1]} writes_equal={ww == gw} calls_equal={want[3] == got_calls}"
                           + (f"\n       writes differ at {[(hex(a), ww.get(a), gw.get(a)) for a in diff]} "
                              "(address, original, recon)" if diff else "")
                           + "".join(f"\n       buffer {i}: original {a.hex()} recon {b.hex()}"
                                     for i, (a, b) in enumerate(zip(want[1], got[1])) if a != b)
                           + (f"\n       calls from #{first_diff(want[3], got_calls)}: original "
                              f"{want[3][first_diff(want[3], got_calls):][:4]!r}\n              recon    "
                              f"{got_calls[first_diff(want[3], got_calls):][:4]!r}" if want[3] != got_calls else ""))
                break
        if verdict == "pass" and not used:
            verdict = f"untestable: the original faults on all {len(case['inputs'])} inputs"
        failed += verdict != "pass" and not verdict.startswith("blocked")
        results.append([build, r["va"], r["symbol"], used, verdict.split("\n")[0]])
        tag = "ok  " if verdict == "pass" else ("UNTESTABLE" if verdict.startswith("untestable") else
                                                "blocked   " if verdict.startswith("blocked") else "FAIL")
        print(f"{tag} {r['va']} {name}  ({used} cases)" + ("" if verdict == "pass" else f"\n     {verdict}"))

    (out / "test-results.tsv").write_text("build\tva\tsymbol\tcases\tresult\n" +
                                          "".join("\t".join(map(str, x)) + "\n" for x in results))
    passed = {x[1] for x in results if x[4] == "pass"}
    # the native replay tests (tests/replay/run.py, run first) cover functions this harness can't reach yet
    replay = ROOT / "build" / build / "replay-results.tsv"
    if not enhanced and replay.exists():
        passed |= {r["va"] for r in symbols.read_tsv(replay) if r["result"] == "pass"}
    if "--check-records" in sys.argv:
        for rec in symbols.read_tsv(ROOT / "records" / "functions.tsv"):
            ran = {x[1] for x in results} | passed  # with -k, only the functions that ran
            if rec["build"] == build and rec["status"] in MATCHED and rec["va"] not in passed and \
                    (not only or rec["va"] in ran):
                print(f"FAIL records claim {rec['status']} for {build}:{rec['va']} without a passing test")
                failed += 1
    print(f"{build}: {len(passed)} passed, {failed} failed, "
          f"{sum(1 for x in results if x[4].startswith('untested'))} without a case, "
          f"{sum(1 for x in results if x[4].startswith('blocked'))} blocked")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
