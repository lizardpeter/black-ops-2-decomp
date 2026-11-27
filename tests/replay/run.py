#!/usr/bin/env python3
"""Run the native replay tests (tests/replay/verify_*.py; see replay.py) against one build's original exe.

    python3 tests/replay/run.py [--build t6mp_msstore] [-k <name>] [originals-dir]

Writes build/<build>/replay-results.tsv (build, va, name, test, result) for tests/differential/run.py
--check-records. Exits non-zero if any test fails.
"""
import importlib.util
import pathlib
import sys
import traceback

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))


def main():
    args = sys.argv[1:]
    build = args[args.index("--build") + 1] if "--build" in args else "t6mp_msstore"
    only = args[args.index("-k") + 1] if "-k" in args else None
    positional = [a for i, a in enumerate(args) if not a.startswith("-") and (i == 0 or args[i - 1] not in ("--build", "-k"))]
    originals = pathlib.Path(positional[0]) if positional else ROOT / "originals"
    exe = originals / f"{build}.exe"
    rows, failed = [], 0
    for path in sorted(HERE.glob("verify_*.py")):
        if only and only not in path.stem:
            continue
        spec = importlib.util.spec_from_file_location(path.stem, path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        funcs = [f for f in mod.FUNCTIONS if f[0] == build]
        if not funcs:
            continue
        try:
            mod.main(exe)
            result = "pass"
        except Exception as e:  # noqa: BLE001 - every failure is reported, the next test still runs
            traceback.print_exc()
            result = f"FAIL {type(e).__name__}"
            failed += 1
        print(f"{'ok  ' if result == 'pass' else 'FAIL'} {path.name}: {', '.join(f[2] for f in funcs)}", flush=True)
        rows += [(b, va, name, path.name, result) for b, va, name in funcs]
    out = ROOT / "build" / build
    out.mkdir(parents=True, exist_ok=True)
    (out / "replay-results.tsv").write_text("build\tva\tname\ttest\tresult\n" +
                                           "".join("\t".join(r) + "\n" for r in rows))
    print(f"{build} replay: {sum(r[4] == 'pass' for r in rows)} functions passed, {failed} tests failed")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
