"""The enhanced Store DLL as it runs inside the game (tools/build_recon.py --build t6mp_msstore --enhanced).

In the game, only the enhanced build's replacements run from the DLL: the loader (runtime/loader) points each
replaced exe function at its replacement. Everything else stays the exe's own code. So inside the DLL:
- exe first: a function src/ defines that the build identifies in the exe (records/names.tsv, anchored matches) is a
  thunk into the exe's copy, ahead of src/'s definition. What the DLL still runs of its own is the replacements and
  what the Store inlined (verified through the Store functions that inline it);
- the redirect table (`t6_redirects`, exported) lists each replacement: the exe function's RVA, the replacement, and
  the first 16 bytes the exe function must start with, which the loader checks before patching anything;
- checks, before anything is written: each replaced exe function is long enough for the 14-byte jump and nothing
  else jumps into those bytes; and no trap (a function or exe data this build doesn't identify yet) is reachable
  from a replacement, which in the game would crash instead of failing a test.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATCH_SIZE = 14  # jmp qword ptr [rip+0]; dq target
PROLOGUE = 16    # bytes the loader checks


def redirect_asm(pe, redirects):
    """The exported table: count, then per replacement RVA, address and the exe function's first bytes."""
    base = pe.OPTIONAL_HEADER.ImageBase
    asm = ["\t.data", "\t.p2align 3", "\t.globl t6_redirects", "t6_redirects:", f"\t.quad {len(redirects)}"]
    for va, sym in redirects:
        head = pe.get_data(va - base, PROLOGUE)
        asm += [f"\t.quad {va - base:#x}", f'\t.quad "{sym}"', "\t.byte " + ", ".join(f"{b:#x}" for b in head)]
    return asm


def check_targets(build, pe, redirects):
    """Each replaced exe function is at least PATCH_SIZE bytes (its .pdata entry) and no branch in the exe lands in
    its first PATCH_SIZE bytes past the entry (the Ghidra disassembly, analysis/<build>/disasm)."""
    import pefile
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_EXCEPTION"]])
    base = pe.OPTIONAL_HEADER.ImageBase
    sizes = {base + e.struct.BeginAddress: e.struct.EndAddress - e.struct.BeginAddress
             for e in getattr(pe, "DIRECTORY_ENTRY_EXCEPTION", [])}
    problems = []
    for va, sym in redirects:
        if sizes.get(va, 0) < PATCH_SIZE:
            problems.append(f"{sym} {va:X}: not a function entry of at least {PATCH_SIZE} bytes in .pdata")
        inside = "|".join(f"0x{a:x}" for a in range(va + 1, va + PATCH_SIZE))
        hits = subprocess.run(["grep", "-rhiE", rf"(J[A-Z]*|CALL)\s+({inside})\b", str(ROOT / "analysis" / build / "disasm")],
                              capture_output=True, text=True).stdout.strip()
        if hits:
            problems.append(f"{sym} {va:X}: the exe branches into its first {PATCH_SIZE} bytes:\n    {hits[:300]}")
    if problems:
        sys.exit("enhanced replacements can't be patched in:\n  " + "\n  ".join(problems))


_MAP_LINE = re.compile(r"^\s*\w+:\w+\s+(\S+)\s+([0-9a-fA-F]{16})\s")


def check_reachable(dll, recon_map, hybrid_rows, roots):
    """No trap is reachable from the replacements. Follows direct calls and jumps, and any reference to the start of a
    function (thread procedures, window procedures), through the linked DLL; a thunk into the exe ends a path."""
    at, addr = {}, {}
    for line in recon_map.read_text().splitlines():
        m = _MAP_LINE.match(line)
        if m:
            at[int(m.group(2), 16)] = m.group(1)
            addr.setdefault(m.group(1), int(m.group(2), 16))
    traps = {}
    for row in hybrid_rows:
        sym, kind = row.split("\t")[:2]
        if kind == "unresolved-function" and sym in addr:
            traps[addr[sym]] = sym
        elif kind == "unresolved-data" and f"t6x@{sym}" in addr:
            traps[addr[f"t6x@{sym}"]] = sym
    starts = sorted(at)
    text = subprocess.run(["llvm-objdump", "-d", "--no-show-raw-insn", str(dll)], capture_output=True,
                          text=True, check=True).stdout
    # instructions go to the function (in the link map) they fall in: the DLL has no symbol table, so the
    # disassembler only labels exports. The build compiles one section per function, so the duplicates
    # /force:multiple loses are dropped (/opt:ref) and every byte of code belongs to a named function.
    import bisect
    body = {a: [] for a in starts}
    for line in text.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\s", line)
        if not m:
            continue
        i = bisect.bisect_right(starts, int(m.group(1), 16)) - 1
        if i >= 0:
            body[starts[i]].append(line)
    # direct branches, rip-relative references (# 0x...) and, with -mcmodel=large, the movabsq $address a call
    # through a register loads
    refs = re.compile(r"(?:callq|jmpq?|j\w+)\s+(?:0x)?([0-9a-f]+)\b|#\s+0x([0-9a-f]+)|\$0x([0-9a-f]+)")
    seen, stack, path = set(), [addr[r] for r in roots], {addr[r]: [r] for r in roots}
    hit = []
    while stack:
        f = stack.pop()
        if f in seen:
            continue
        seen.add(f)
        if f in traps:
            hit.append(" -> ".join(path[f]) + f"  (trap: {traps[f]})")
            continue
        for line in body.get(f, []):
            for a, b, c in refs.findall(line):
                t = int(a or b or c, 16)
                if t in traps and t not in seen:
                    path[t] = path[f] + [traps[t]]
                    stack.append(t)
                elif t in body and t != f and t not in seen:
                    path.setdefault(t, path[f] + [at.get(t, hex(t))])
                    stack.append(t)
    if hit:
        sys.exit("enhanced replacements reach code the build doesn't identify (it would crash in the game):\n  "
                 + "\n  ".join(hit))
    return sum(1 for f in seen if body.get(f))  # code, not the data it reads
