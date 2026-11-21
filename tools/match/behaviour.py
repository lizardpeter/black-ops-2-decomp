#!/usr/bin/env python3
"""Find a reconstructed function in a build without symbols by its behaviour.

    python3 tools/match/behaviour.py <build> <original-exe> <library.so> [name ...]

For small self-contained functions (leaves with no string or call-graph anchor, which tools/match/match.py cannot
place), every candidate entry in the original is run under Unicorn on probe inputs and compared with the
reconstruction compiled from src/ (tests/differential/reference.py's FUNCTIONS and cases). A candidate that agrees on
every probe is then run on the full differential case set. Candidates are the build's .pdata function starts (x64),
every direct call target, and every 16-byte-aligned entry after int3 padding; x64 leaf functions that touch no stack
have no .pdata entry, and some are reached only through pointers or tail jumps.

Writes analysis/<build>/match/behaviour.tsv (generated): name, va, size, candidates tried, probe survivors, verdict.
A unique `unique` verdict is the evidence for a records/names.tsv row (method `behaviour+differential`).
"""
import collections
import csv
import ctypes
import hashlib
import struct
import sys
import tomllib
from pathlib import Path

import pefile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests" / "differential"))
import reference  # noqa: E402  (FUNCTIONS, cases, Emulator)


def candidates(pe):
    base = pe.OPTIONAL_HEADER.ImageBase
    text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    lo, data = base + text.VirtualAddress, text.get_data()
    hi = lo + len(data)
    found = collections.Counter()
    for i in range(len(data) - 5):
        if data[i] == 0xE8:
            t = lo + i + 5 + struct.unpack_from("<i", data, i + 1)[0]
            if lo <= t < hi:
                found[t] += 1
    # 16-byte-aligned entries after int3 padding: functions reached only through pointers or tail jumps
    for i in range(16, len(data), 16):
        if data[i - 1] == 0xCC and data[i] != 0xCC:
            found[lo + i] += 1
    if pe.FILE_HEADER.Machine == 0x8664:
        pdata = next((s for s in pe.sections if s.Name.rstrip(b"\0") == b".pdata"), None)
        if pdata:
            d = pdata.get_data()[:pdata.Misc_VirtualSize]
            for i in range(0, len(d) - 11, 12):
                begin = struct.unpack_from("<I", d, i)[0]
                if begin:
                    found[base + begin] += 1
    return sorted(found)


def main():
    build_id, exe, lib = sys.argv[1], Path(sys.argv[2]), Path(sys.argv[3])
    names = sys.argv[4:] or list(reference.FUNCTIONS)
    cfg = tomllib.loads((ROOT / "target.toml").read_text())
    build = next(b for b in cfg["build"] if b["id"] == build_id)
    binary = exe.read_bytes()
    if hashlib.sha256(binary).hexdigest() != build["exe"]["sha256"]:
        sys.exit(f"{exe}: sha256 does not match target.toml for {build_id}")
    pe = pefile.PE(data=binary)
    emu = reference.Emulator(pe)
    library = ctypes.CDLL(str(lib.resolve()))
    starts = candidates(pe)
    print(f"{build_id}: {len(starts):,} candidate entries")
    rows = []
    for name in names:
        _, _, restype, argtypes = reference.FUNCTIONS[name]
        fn = getattr(library, name)
        fn.restype, fn.argtypes = restype, argtypes
        all_cases = list(reference.cases(name))
        expected = [reference.call_reconstructed(fn, name, args) for args in all_cases]
        # probes: a spread of cases, cheap to reject most candidates on the first
        step = max(1, len(all_cases) // 24)
        probe_idx = list(range(0, len(all_cases), step))[:24]
        survivors = []
        for va in starts:
            ok = True
            for i in probe_idx:
                got = emu.call(va, name, all_cases[i], restype, max_insns=20_000)
                if got is None or got != expected[i]:
                    ok = False
                    break
            if ok:
                survivors.append(va)
        full = [va for va in survivors
                if all(emu.call(va, name, a, restype, max_insns=2_000_000) == e for a, e in zip(all_cases, expected))]
        verdict = "unique" if len(full) == 1 else ("none" if not full else "ambiguous")
        sizes = [emu.extent(va) for va in full]
        print(f"{name}: {len(survivors)} probe survivor(s), {len(full)} full match(es): "
              + ", ".join(f"{va:08X} ({sz} bytes)" for va, sz in zip(full, sizes)) + f" -> {verdict}")
        for va, sz in zip(full, sizes) or [(None, 0)]:
            rows.append([name, f"{va:08X}" if va else "", str(sz), str(len(starts)), str(len(survivors)), verdict])
    out = ROOT / "analysis" / build_id / "match"
    out.mkdir(parents=True, exist_ok=True)
    path = out / "behaviour.tsv"
    old = {}
    if path.exists():
        with open(path, newline="") as f:
            for r in csv.DictReader(f, delimiter="\t"):
                old.setdefault(r["name"], []).append([r[k] for k in ("name", "va", "size", "candidates", "survivors", "verdict")])
    for n in names:
        old.pop(n, None)
    for r in rows:
        old.setdefault(r[0], []).append(r)
    with open(path, "w", newline="\n") as f:
        f.write("name\tva\tsize\tcandidates\tsurvivors\tverdict\n")
        for n in sorted(old):
            for r in old[n]:
                f.write("\t".join(r) + "\n")


if __name__ == "__main__":
    main()
