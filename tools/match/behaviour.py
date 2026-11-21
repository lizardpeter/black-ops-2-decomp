#!/usr/bin/env python3
"""Find a reference function in a build without symbols by its behaviour.

    python3 tools/match/behaviour.py <build> [name ...]

For small self-contained functions (leaves with no string or call-graph anchor, which tools/match/match.py cannot
place). The oracle is the reference build's ORIGINAL function, run in its own emulator on the inputs of its
differential case (tests/differential/test_*.py, by name). Every candidate entry in the target build is run in a
second emulator on probe inputs; a candidate that agrees on every probe is then run on all of them. Return values
and buffers are compared; writes to globals are not, since the two builds' globals live at different addresses.
Candidates are the target's .pdata function starts (x64), every direct call target, and every 16-byte-aligned entry
after int3 padding: x64 leaf functions that touch no stack have no .pdata entry, and some are reached only through
pointers or tail jumps.

Writes analysis/<build>/match/behaviour.tsv (generated): name, va, size, candidates tried, probe survivors, verdict.
A `unique` verdict is the evidence for a records/names.tsv row (method `behaviour+differential`).
"""
import collections
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests" / "differential"))
import symbols  # noqa: E402
from harness import Image, Machine, original_exe  # noqa: E402
from run import load_cases, same  # noqa: E402

PROBE_LIMIT = 20_000
FULL_LIMIT = 20_000_000


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


def extent(m, img, va):
    """Bytes from va to the end of its .pdata range (x64), else to the first ret followed by padding."""
    pe = img.pe
    if img.x64:
        sec = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".pdata")
        d = sec.get_data()[:sec.Misc_VirtualSize]
        for i in range(0, len(d) - 11, 12):
            b, e = struct.unpack_from("<II", d, i)
            if img.base + b == va:
                return e - b
    code = bytes(m.mu.mem_read(va, 0x1000))
    return next((i + 1 for i in range(len(code) - 1) if code[i] == 0xC3 and code[i + 1] in (0xCC, 0x90)), 0)


def main():
    build, names = sys.argv[1], sys.argv[2:]
    ref = symbols.reference_build()
    cases = load_cases()
    names = names or sorted(cases)
    ref_m = Machine(Image(original_exe(ref)))
    target = Image(original_exe(build))
    tm = Machine(target)
    starts = candidates(target.pe)
    print(f"{build}: {len(starts):,} candidate entries; oracle {ref}")
    rows = []
    for name in names:
        case = cases[name]
        hit = next((va for s, (va, _) in symbols.table(ref).items() if symbols.scoped_name(s) == name), None)
        if hit is None:
            print(f"{name}: not in {ref}")
            continue
        inputs, expected = [], []
        for inp in case["inputs"]:
            args, setup = (inp["args"], inp.get("setup")) if isinstance(inp, dict) else (inp, None)
            want = ref_m.call(hit, args, case["ret"], setup)
            if not (isinstance(want[0], str) and want[0].startswith("fault")):
                inputs.append((args, setup))
                expected.append(want[:2])
        step = max(1, len(inputs) // 24)
        probes = list(range(0, len(inputs), step))[:24]

        def agrees(va, idx, limit):
            for i in idx:
                got = tm.call(va, inputs[i][0], case["ret"], inputs[i][1], limit=limit)
                if isinstance(got[0], str) or not same(got[0], expected[i][0]) or got[1] != expected[i][1]:
                    return False
            return True
        survivors = [va for va in starts if agrees(va, probes, PROBE_LIMIT)]
        full = [va for va in survivors if agrees(va, range(len(inputs)), FULL_LIMIT)]
        verdict = "unique" if len(full) == 1 else ("none" if not full else "ambiguous")
        sizes = [extent(tm, target, va) for va in full]
        print(f"{name}: {len(survivors)} probe survivor(s), {len(full)} full match(es): "
              + ", ".join(f"{va:08X} ({sz} bytes)" for va, sz in zip(full, sizes)) + f" -> {verdict}")
        for va, sz in (list(zip(full, sizes)) or [(None, 0)]):
            rows.append([name, f"{va:08X}" if va else "", str(sz), str(len(starts)), str(len(survivors)), verdict])

    path = ROOT / "analysis" / build / "match" / "behaviour.tsv"
    path.parent.mkdir(parents=True, exist_ok=True)
    keep = [r for r in symbols.read_tsv(path) if r["name"] not in names]
    out = [[r[k] for k in ("name", "va", "size", "candidates", "survivors", "verdict")] for r in keep] + rows
    path.write_text("name\tva\tsize\tcandidates\tsurvivors\tverdict\n" +
                    "".join("\t".join(r) + "\n" for r in sorted(out)))


if __name__ == "__main__":
    main()
