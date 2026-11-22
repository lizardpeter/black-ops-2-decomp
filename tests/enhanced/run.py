#!/usr/bin/env python3
"""Checks for the enhanced build's patches (records/patches.tsv).

    python3 tests/enhanced/run.py [--build <id>] [-k <patch>]

tests/differential/run.py --enhanced already shows that every function the enhanced build doesn't override still
matches the original. The overrides are checked here, against what their patch promises: each test_*.py lists
CHECKS, functions taking a Context and raising AssertionError on a broken promise. A check usually runs the
original and the enhanced function on the same inputs and compares everything except what the patch changes.
"""
import importlib.util
import sys
import traceback
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests" / "differential"))
import symbols  # noqa: E402
from harness import Image, Machine, original_exe  # noqa: E402
import run as differential  # noqa: E402


class Context:
    def __init__(self, build):
        self.build = build
        self.recon = Image(ROOT / "build" / build / "enhanced" / "recon.dll")
        self.m = Machine(Image(original_exe(build)), self.recon)
        crt = [symbols.resolve(build, n) for n in ("__getptd", "__getptd_noexit", "___ptmbcinfo", "___ptlocinfo")]
        if not self.recon.x64 and all(crt):
            self.m.crt_thread_data([crt[0][0], crt[1][0]], crt[2][0], crt[3][0])
        # the enhanced DLL's addresses made comparable with the original's (as tests/differential/run.py does):
        # its copies of original functions, its thunks into the exe, its string literals
        rows = symbols.read_tsv(ROOT / "build" / build / "enhanced" / "recon.symbols.tsv")
        recon_to_orig = {self.recon.exports[r["symbol"]]: int(r["va"], 16) for r in rows
                         if r["va"] and r["symbol"] in self.recon.exports}
        self.canon = differential.Canon(self.m, self.recon, recon_to_orig)

    def original(self, name):
        """Address of the original function `name` (scoped plain name)."""
        return next(va for s, (va, k) in symbols.table(self.build).items()
                    if k == "function" and symbols.scoped_name(s) == name)

    def enhanced(self, name):
        """Address of the enhanced build's `name`."""
        return next(va for e, va in self.recon.exports.items() if symbols.scoped_name(e) == name)

    def hooks(self, spec):
        return differential.resolve_hooks(spec, self.build, self.m, self.recon)

    def imports(self, spec):
        for name, (fn, nargs) in spec.items():
            self.m.on_import(name, (lambda f: lambda mu: f(self.m, mu))(fn), nargs)


def main():
    build = sys.argv[sys.argv.index("--build") + 1] if "--build" in sys.argv else symbols.reference_build()
    only = sys.argv[sys.argv.index("-k") + 1] if "-k" in sys.argv else None
    ctx = Context(build)
    failed = passed = 0
    for path in sorted(Path(__file__).parent.glob("test_*.py")):
        spec = importlib.util.spec_from_file_location(path.stem, path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        if getattr(mod, "BUILDS", None) and build not in mod.BUILDS:
            continue
        for check in getattr(mod, "CHECKS", []):
            if only and only not in check.__name__ and only != getattr(mod, "PATCH", ""):
                continue
            try:
                check(ctx)
                passed += 1
                print(f"ok   {getattr(mod, 'PATCH', '')} {check.__name__}")
            except AssertionError as e:
                failed += 1
                print(f"FAIL {getattr(mod, 'PATCH', '')} {check.__name__}: {e}")
            except Exception:
                failed += 1
                print(f"FAIL {getattr(mod, 'PATCH', '')} {check.__name__}:\n{traceback.format_exc()}")
    print(f"{build} enhanced: {passed} checks passed, {failed} failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
