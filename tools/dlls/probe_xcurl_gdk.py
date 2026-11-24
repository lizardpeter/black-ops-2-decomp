#!/usr/bin/env python3
"""Read-only XCurl GDK interface evidence from the hash-pinned Store DLL.

Prints the raw GUID-like data at the three known interface RVAs, every RIP-relative
or immediate code reference to those RVAs, and complete unwind-bounded functions.
The original DLL is parsed as data only; no code is executed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import uuid
from pathlib import Path

import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

SHA256 = "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"
TARGETS = (0x20120, 0x20130, 0x20140)
KNOWN = (0x5148, 0x5364, 0x53A4, 0x11978, 0x11F10)
EXPORTS = (
    "QueryApiImpl", "InitializeApiImpl", "InitializeApiImplEx",
    "XErrorReport", "UninitializeApiImpl",
)

def ranges(pe):
    return {entry.struct.BeginAddress: entry.struct.EndAddress
            for entry in pe.DIRECTORY_ENTRY_EXCEPTION}

def containing(table, rva):
    found = [(start, end) for start, end in table.items() if start <= rva < end]
    if len(found) != 1:
        raise ValueError(f"{rva:#x}: expected one .pdata range, got {found}")
    return found[0]

def section_name(pe, rva):
    section = pe.get_section_by_rva(rva)
    return section.Name.rstrip(b"\0").decode("ascii", "replace") if section else None

def disassemble(md, pe, start, end):
    code = pe.get_data(start, end - start)
    return list(md.disasm(code, pe.OPTIONAL_HEADER.ImageBase + start))

def fmt(ins, base):
    return f"{ins.address-base:08x}  {ins.mnemonic:8s} {ins.op_str}"

def target_references(pe, table, md):
    base = pe.OPTIONAL_HEADER.ImageBase
    targets = set(TARGETS)
    found = {target: [] for target in TARGETS}
    for start, end in table.items():
        for ins in disassemble(md, pe, start, end):
            refs = set()
            for op in ins.operands:
                if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                    refs.add(ins.address + ins.size + op.mem.disp - base)
                elif op.type == CS_OP_IMM:
                    value = op.imm
                    refs.add(value - base if value >= base else value)
            for target in targets.intersection(refs):
                found[target].append((start, end, ins))
    return found

def read_cstring(pe, rva, limit=256):
    blob = pe.get_data(rva, limit)
    return blob.split(b"\\0", 1)[0].decode("utf-8", "backslashreplace")

def read_wstring(pe, rva, limit=512):
    blob = pe.get_data(rva, limit)
    blob = blob[:len(blob) - (len(blob) % 2)]
    end = next((i for i in range(0, len(blob), 2) if blob[i:i+2] == b"\\0\\0"), len(blob))
    return blob[:end].decode("utf-16le", "backslashreplace")

def raw_strings(data, pe, needles):
    results = {}
    for label, needle in needles.items():
        rows = []
        pos = 0
        while (pos := data.find(needle, pos)) >= 0:
            # Translate file offset to RVA using the containing section.
            rva = None
            for section in pe.sections:
                lo = section.PointerToRawData
                hi = lo + section.SizeOfRawData
                if lo <= pos < hi:
                    rva = section.VirtualAddress + pos - lo
                    break
            rows.append({"file_offset": hex(pos), "rva": hex(rva) if rva is not None else None})
            pos += 1
        results[label] = rows
    return results

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dll", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    data = args.dll.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SHA256:
        raise SystemExit(f"wrong original XCurl.dll SHA-256: {digest}")
    pe = pefile.PE(data=data, fast_load=False)
    if pe.FILE_HEADER.Machine != 0x8664 or pe.OPTIONAL_HEADER.Magic != 0x20B:
        raise SystemExit("expected AMD64 PE32+")
    base = pe.OPTIONAL_HEADER.ImageBase
    table = ranges(pe)
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    md.detail = True

    targets = {}
    for rva in TARGETS:
        blob = pe.get_data(rva, 16)
        if len(blob) != 16:
            raise SystemExit(f"truncated target data at {rva:#x}")
        targets[hex(rva)] = {
            "section": section_name(pe, rva),
            "bytes": blob.hex(),
            "guid_interpretation": str(uuid.UUID(bytes_le=blob)),
            "raw_u32": [hex(int.from_bytes(blob[i:i+4], "little")) for i in range(0, 16, 4)],
        }

    references = target_references(pe, table, md)
    xrefs = {}
    for target, rows in references.items():
        xrefs[hex(target)] = []
        for start, end, ins in rows:
            item = {"function": hex(start), "end": hex(end), "instruction": fmt(ins, base)}
            if item not in xrefs[hex(target)]:
                xrefs[hex(target)].append(item)

    xref_functions = {}
    for rows in references.values():
        for start, end, _ in rows:
            xref_functions[start] = end
    referenced = {}
    for start, end in sorted(xref_functions.items()):
        ins = disassemble(md, pe, start, end)
        direct_calls = sorted({
            op.imm - base for insn in ins if insn.mnemonic == "call"
            for op in insn.operands if op.type == CS_OP_IMM
        })
        referenced[hex(start)] = {
            "end": hex(end),
            "direct_calls": [hex(x) for x in direct_calls],
            "instructions": [fmt(insn, base) for insn in ins],
        }

    data_strings = {hex(rva): read_cstring(pe, rva) for rva in
                    (0x1FF78, 0x1FF90, 0x1FFA0, 0x1FFB8, 0x1FFD0, 0x1FFE0)}
    wide_strings = {hex(rva): read_wstring(pe, rva) for rva in
                    (0x20320, 0x20340, 0x20360, 0x20378, 0x203A0, 0x203C0)}

    known = {}
    for rva in KNOWN:
        start, end = containing(table, rva)
        known[hex(rva)] = {
            "function": hex(start), "end": hex(end),
            "instructions": [fmt(ins, base) for ins in disassemble(md, pe, start, end)],
        }

    imports = {}
    if hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            dll = entry.dll.decode("ascii", "replace")
            names = [imp.name.decode("ascii", "replace") if imp.name else f"ordinal_{imp.ordinal}"
                     for imp in entry.imports]
            imports[dll] = names
    needles = {name: name.encode() for name in EXPORTS}
    needles["xgameruntime.dll"] = b"xgameruntime.dll"
    needles["xgameruntime.dll_utf16"] = "xgameruntime.dll".encode("utf-16le")
    strings = raw_strings(data, pe, needles)

    result = {
        "sha256": digest, "image_base": hex(base),
        "machine": hex(pe.FILE_HEADER.Machine),
        "exception_function_count": len(table),
        "interface_data": targets, "code_references": xrefs,
        "interface_reference_functions": referenced,
        "known_gdk_functions": known, "data_strings": data_strings,
        "wide_strings": wide_strings, "imports": imports,
        "dynamic_loader_strings": strings,
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")

    print(f"XCurl SHA-256 verified; .pdata functions: {len(table)}")
    for target, detail in targets.items():
        print(f"\nINTERFACE DATA {target} section={detail['section']} "
              f"bytes={detail['bytes']} guid-interpretation={detail['guid_interpretation']}")
        for ref in xrefs[target]:
            print(f"XREF function {ref['function']}..{ref['end']}: {ref['instruction']}")
    for rva, fn in known.items():
        print(f"\nKNOWN GDK FUNCTION {rva} range={fn['function']}..{fn['end']}")
        print("\n".join(fn["instructions"]))
    for rva, fn in referenced.items():
        print(f"\nINTERFACE REFERENCE FUNCTION {rva}..{fn['end']} "
              f"direct_calls={','.join(fn['direct_calls'])}")
        print("\n".join(fn["instructions"]))
    print("\nDYNAMIC LOADER STRINGS", json.dumps(strings, sort_keys=True))
    print("\nREFERENCED DATA STRINGS", json.dumps(data_strings, sort_keys=True))
    print("\nWIDE DATA STRINGS", json.dumps(wide_strings, sort_keys=True))
    print("\nDYNAMIC IMPORTS")
    for dll, names in imports.items():
        selected = [name for name in names if any(key in name.lower() for key in
                    ("loadlibrary", "getprocaddress", "srwlock", "debugbreak", "getlasterror"))]
        if selected:
            print(dll, ", ".join(selected))
    print(f"\nEvidence JSON: {args.out}")

if __name__ == "__main__":
    main()
