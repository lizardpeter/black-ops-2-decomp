#!/usr/bin/env python3
"""Extract exact MP renderer registration evidence from the hash-pinned PE.

This is static analysis, not a reconstructed body or a runtime replay.
Addresses are original image VAs. Only file-backed bytes are decoded as data;
zero-filled globals are identified but never presented as runtime values.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import struct

import capstone as cs
from capstone import x86_const as x
import pefile

ROOT = Path(__file__).resolve().parents[2]
DIGEST = "cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01"
START, END = 0x14070E230, 0x1407175D8
TARGETS = {0x14046B8B0: "find", 0x14046C1A0: "new",
           0x14046BF10: "reregister", 0x14046C4C0: "bool",
           0x14046C580: "int", 0x14046C650: "float",
           0x14046C760: "vec2", 0x14046C840: "vec3",
           0x14046C930: "vec4", 0x14046CA30: "string",
           0x14046CAF0: "enum", 0x14046CBD0: "color"}


def extract(path, out):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != DIGEST:
        raise ValueError("Store MP original hash mismatch")
    pe = pefile.PE(data=raw, fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    assert base == 0x140000000

    def read(va, count):
        rva = va - base
        if rva < 0:
            return None
        section = pe.get_section_by_rva(rva)
        if section is None:
            return None
        off = rva - section.VirtualAddress
        if off < 0 or off + count > section.SizeOfRawData:
            return None
        pos = section.PointerToRawData + off
        if pos + count > len(raw):
            return None
        return raw[pos:pos + count]

    def string(va):
        buf = bytearray()
        for offset in range(512):
            byte = read(va + offset, 1)
            if byte is None:
                return None
            if byte == b"\0":
                return buf.decode("ascii") if buf else None
            if not 32 <= byte[0] < 127:
                return None
            buf.extend(byte)
        return None

    def reference(va, width):
        section = pe.get_section_by_rva(va - base) if va >= base else None
        result = {"va": hex(va), "width": width}
        if section is None:
            return result
        result["section"] = section.Name.rstrip(b"\0").decode("ascii")
        result["writable"] = bool(section.Characteristics & 0x80000000)
        data = read(va, width) if width else None
        if data is not None:
            result["file_bytes"] = data.hex()
            if width == 4:
                result["u32"] = hex(struct.unpack("<I", data)[0])
                result["f32_repr"] = repr(struct.unpack("<f", data)[0])
        if not result["writable"]:
            result["string_candidate"] = string(va)
        # Several renderer enum arrays are initialized .data, not .rdata.
        # Recover file-backed pointer tables regardless of section flags.
        # This is initial image evidence, not proof of unchanged runtime data.
        if width == 0:
            entries = []
            for i in range(128):
                qword = read(va + i * 8, 8)
                if qword is None:
                    break
                ptr, = struct.unpack("<Q", qword)
                if ptr == 0:
                    if entries:
                        result["terminated_string_pointer_table_candidate"] = entries
                    break
                name = string(ptr)
                if name is None:
                    break
                entries.append({"va": hex(ptr), "text": name})
        return result

    body = read(START, END - START)
    assert body is not None
    md = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_64)
    md.detail = True
    insns = list(md.disasm(body, START))
    if not insns or insns[-1].address + insns[-1].size != END:
        raise ValueError("incomplete decode")
    assert sum(i.size for i in insns) == len(body)
    addresses = {i.address for i in insns}
    leaders = {START}
    calls = []
    branches = []
    refs = {}
    rows = []
    for i in insns:
        next_va = i.address + i.size
        rip = []
        for op in i.operands:
            if op.type == x.X86_OP_MEM and op.mem.base == x.X86_REG_RIP:
                va = next_va + op.mem.disp
                width = 0 if i.mnemonic == "lea" else op.size
                key = (va, width)
                if key not in refs:
                    refs[key] = reference(va, width)
                rip.append({"va": hex(va), "width": width,
                            "access": op.access})
        direct = (i.operands[0].imm if i.operands and
                  i.operands[0].type == x.X86_OP_IMM else None)
        target = None
        if i.group(cs.CS_GRP_CALL):
            target = direct
            calls.append({"va": hex(i.address), "target": hex(target) if target is not None else None,
                          "kind": TARGETS.get(target, "other" if target is not None else "indirect"),
                          "operand": i.op_str})
        if i.group(cs.CS_GRP_JUMP) or i.group(cs.CS_GRP_RET):
            target = direct if i.group(cs.CS_GRP_JUMP) else None
            if target is not None and START <= target < END:
                if target not in addresses:
                    raise ValueError(f"branch into undecoded instruction: {target:x}")
                leaders.add(target)
            if next_va in addresses:
                leaders.add(next_va)
            branches.append({"va": hex(i.address), "op": i.mnemonic,
                             "target": hex(target) if target is not None else None,
                             "fallthrough": hex(next_va) if i.mnemonic not in ("jmp", "ret", "retf") else None})
        rows.append({"va": hex(i.address), "bytes": i.bytes.hex(), "op": i.mnemonic,
                     "args": i.op_str, "rip": rip})
    with (ROOT / "docs/store-renderer-dvar-registration.tsv").open() as f:
        expected = {(int(r["instruction"], 16), int(r["target"], 16), r["kind"])
                    for r in csv.DictReader(f, delimiter="\t")}
    actual = {(int(c["va"], 16), int(c["target"], 16), c["kind"])
              for c in calls if c["kind"] in TARGETS.values()}
    if actual != expected:
        raise ValueError(f"inventory mismatch: missing={expected-actual}, extra={actual-expected}")
    # Explicitly recover the two unresolved operands; dynamic table length is
    # not inferred from this bounded raw pointer window.
    unresolved = {"14093c480": string(0x14093C480), "1409ad3f0": []}
    for index in range(64):
        va = 0x1409AD3F0 + index * 8
        data = read(va, 8)
        if data is None:
            break
        pointer, = struct.unpack("<Q", data)
        unresolved["1409ad3f0"].append({"index": index, "pointer": hex(pointer),
                                       "string_candidate": string(pointer)})
    out.mkdir(parents=True, exist_ok=True)
    evidence = {"sha256": DIGEST, "start": hex(START), "end_exclusive": hex(END),
                "instructions": rows, "calls": calls, "branches": branches,
                "block_leaders": [hex(a) for a in sorted(leaders)],
                "references": list(refs.values()), "unresolved_operands": unresolved}
    # Explicit table arguments recovered from the contiguous source prefix
    # and subsequent renderer debug/material registration instructions.
    table_vas = {
        "r_clipCodec": 0x140D39F30, "r_clipSize": 0x140D3A1E0,
        "r_texFilterMipMode": 0x140D3A138, "r_debugShader": 0x140D3A1A8,
        "r_debugPerformance": 0x140D3A230, "r_lightMapSecondary": 0x140D39F50,
        "r_colorMap": 0x140D3A088, "r_alphaMap": 0x140D3A0B8,
        "r_specularMap": 0x140D3A250, "r_glossMap": 0x140D3A270,
        "r_occlusionMap": 0x140D3A208,
    }
    evidence["enum_table_arguments"] = {
        name: reference(va, 0) for name, va in table_vas.items()
    }
    (out / "renderer-enum-tables.json").write_text(
        json.dumps(evidence["enum_table_arguments"], indent=2) + "\n")
    print("Renderer enum table evidence:", json.dumps(evidence["enum_table_arguments"]))
    (out / "renderer-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
    lookup = {(r["va"], r["width"]): r for r in refs.values()}
    lines = []
    for row in rows:
        annotations = [json.dumps(lookup[(r["va"], r["width"])], separators=(",", ":"))
                       for r in row["rip"]]
        lines.append(f'{row["va"]} {row["bytes"]:24} {row["op"]:10} {row["args"]}' +
                     (" ; " + " ".join(annotations) if annotations else ""))
    (out / "renderer-annotated.asm").write_text("\n".join(lines) + "\n")
    print(f"{len(body)} bytes; {len(rows)} instructions; {len(leaders)} block leaders; "
          f"{len(calls)} calls, {len(actual)} exact inventory matches; {len(refs)} RIP references")
    print("Unresolved operands, bounded raw evidence:", json.dumps(unresolved))
    print("Renderer entry evidence:")
    for line in lines[:145]:
        print(line)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("out", type=Path)
    args = parser.parse_args()
    extract(args.exe, args.out)


if __name__ == "__main__":
    main()
