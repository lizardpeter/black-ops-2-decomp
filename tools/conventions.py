#!/usr/bin/env python3
"""Find functions whose original takes arguments in registers (link-time code generation's custom conventions).

    python3 tools/conventions.py <build> <source file> ...   e.g. CoDMPServer_PC client/cl_input.cpp

For each function of the given source files (analysis/<build>/functions.tsv), computes which registers are live at
its entry: read on some path before being written. EBP and ESP are ignored, and so are pushes of EBX, ESI, EDI and
EBP (the prologue saving them). A call is taken to read nothing and to clobber EAX, ECX, EDX and XMM0-XMM7, which
undercounts a register passed straight through to a callee with its own convention: check the disassembly.

Writes analysis/<build>/conventions.tsv (generated): va, name, live-in registers, stack argument bytes the function
reads ([EBP+8] upwards, or [ESP+4] upwards before the frame is set up). records/conventions.tsv (authored) maps
those registers onto the decorated prototype's parameters.
"""
import collections
import csv
import struct
import sys
from pathlib import Path

import capstone
import pefile
from capstone import x86_const as X

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import symbols  # noqa: E402

GPR = {X.X86_REG_EAX: "eax", X.X86_REG_ECX: "ecx", X.X86_REG_EDX: "edx", X.X86_REG_EBX: "ebx",
       X.X86_REG_ESI: "esi", X.X86_REG_EDI: "edi", X.X86_REG_AX: "eax", X.X86_REG_AL: "eax", X.X86_REG_AH: "eax",
       X.X86_REG_CX: "ecx", X.X86_REG_CL: "ecx", X.X86_REG_CH: "ecx", X.X86_REG_DX: "edx", X.X86_REG_DL: "edx",
       X.X86_REG_DH: "edx", X.X86_REG_BX: "ebx", X.X86_REG_BL: "ebx", X.X86_REG_BH: "ebx", X.X86_REG_SI: "esi",
       X.X86_REG_DI: "edi"}
for i in range(8):
    GPR[getattr(X, f"X86_REG_XMM{i}")] = f"xmm{i}"
CLOBBERED = {"eax", "ecx", "edx"} | {f"xmm{i}" for i in range(8)}
SAVED = {X.X86_REG_EBX, X.X86_REG_ESI, X.X86_REG_EDI, X.X86_REG_EBP}


def regs(insn, which):
    read, written = insn.regs_access()
    return {GPR[r] for r in (read if which == "r" else written) if r in GPR}


def analyse(pe, va, size):
    base = pe.OPTIONAL_HEADER.ImageBase
    code = pe.get_data(va - base, size)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    insns = {i.address: i for i in md.disasm(code, va)}
    order = sorted(insns)
    succ = collections.defaultdict(list)
    for n, a in enumerate(order):
        i = insns[a]
        nxt = order[n + 1] if n + 1 < len(order) else None
        if i.mnemonic in ("ret", "retn"):
            continue
        if i.group(capstone.CS_GRP_JUMP):
            op = i.operands[0]
            if op.type == capstone.CS_OP_IMM and op.imm in insns:
                succ[a].append(op.imm)
            if i.mnemonic != "jmp" and nxt:
                succ[a].append(nxt)
            continue
        if i.mnemonic == "int3":
            continue
        if nxt:
            succ[a].append(nxt)
    # backward liveness per instruction
    live = {a: set() for a in order}
    changed = True
    while changed:
        changed = False
        for a in reversed(order):
            i = insns[a]
            out = set().union(*(live[s] for s in succ[a])) if succ[a] else set()
            if i.mnemonic == "call":
                use, defs = set(), CLOBBERED
            elif i.mnemonic == "push" and i.operands[0].type == capstone.CS_OP_REG and i.operands[0].reg in SAVED:
                use, defs = set(), set()
            elif i.mnemonic in ("xor", "sub", "pxor", "xorps") and len(i.operands) == 2 and \
                    i.operands[0].type == i.operands[1].type == capstone.CS_OP_REG and \
                    i.operands[0].reg == i.operands[1].reg:
                use, defs = set(), regs(i, "w")  # zeroing idiom
            else:
                use, defs = regs(i, "r"), regs(i, "w")
            new = use | (out - defs)
            if new != live[a]:
                live[a], changed = new, True
    stack = 0
    for i in insns.values():
        for op in i.operands:
            if op.type == capstone.CS_OP_MEM and op.mem.base == X.X86_REG_EBP and op.mem.disp >= 8:
                stack = max(stack, op.mem.disp - 8 + op.size)
    return sorted(live[order[0]]) if order else [], stack


def main():
    build, files = sys.argv[1], [f.replace("\\", "/").lower() for f in sys.argv[2:]]
    pe = pefile.PE(str(ROOT / "originals" / f"{build}.exe"), fast_load=True)
    rows = [r for r in symbols.read_tsv(ROOT / "analysis" / build / "functions.tsv")
            if any(r["source"].replace("\\", "/").lower().endswith(f) for f in files)]
    out = []
    for r in sorted(rows, key=lambda r: r["va"]):
        live, stack = analyse(pe, int(r["va"], 16), int(r["size"]))
        out.append([r["va"], r["pdb_name"], ",".join(live), str(stack)])
        print(f"{r['va']} {r['pdb_name']:40} live-in: {','.join(live) or '-':24} stack bytes: {stack}")
    path = ROOT / "analysis" / build / "conventions.tsv"
    old = [x for x in symbols.read_tsv(path)] if path.exists() else []
    keep = [[x["va"], x["name"], x["live_in"], x["stack_bytes"]] for x in old if x["va"] not in {o[0] for o in out}]
    rows = sorted(keep + out)
    path.write_text("va\tname\tlive_in\tstack_bytes\n" + "".join("\t".join(x) + "\n" for x in rows))


if __name__ == "__main__":
    main()
