#!/usr/bin/env python3
"""Write a PE inventory for one hash-pinned original without loading it.

    python3 tools/inventory.py <build> <exe>

Writes analysis/<build>/pe.json, and for x64 builds analysis/<build>/pdata.tsv: one row per function from the
exception directory (.pdata), the compiler's own record of every non-leaf function's bounds. Entries whose unwind info
is chained (UNW_FLAG_CHAININFO) are separated-out parts of another function and are listed with `part_of`, not counted
as functions. Leaf functions that never touch the stack have no entry; Ghidra's inventory supersedes this table.
"""
import argparse
import hashlib
import json
import struct
import uuid
from pathlib import Path

import pefile

from model import ROOT, config


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build")
    parser.add_argument("exe", type=Path)
    args = parser.parse_args()
    build = next(item for item in config()["build"] if item["id"] == args.build)
    data = args.exe.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != build["exe"]["sha256"]:
        raise ValueError(f"{args.build}: executable SHA-256 mismatch")
    pe = pefile.PE(data=data)
    machine = {"x86": 0x14c, "x64": 0x8664}[build.get("architecture", "x86")]
    if pe.FILE_HEADER.Machine != machine or pe.OPTIONAL_HEADER.ImageBase != build["image_base"]:
        raise ValueError(f"{args.build}: expected {build.get('architecture', 'x86')} PE at the pinned image base")

    codeview = next(entry.entry for entry in pe.DIRECTORY_ENTRY_DEBUG
                    if getattr(getattr(entry, "entry", None), "CvSignature", None) == b"RSDS")
    pdb_guid = str(uuid.UUID(bytes_le=codeview.__pack__()[4:20])).upper()
    if (pdb_guid, codeview.Age) != (build["pdb_guid"], build["pdb_age"]):
        raise ValueError(f"{args.build}: CodeView identity mismatch")

    metadata = {
        "build": args.build,
        "sha256": digest,
        "size": len(data),
        "image_base": f"{pe.OPTIONAL_HEADER.ImageBase:08X}",
        "entry_point": f"{pe.OPTIONAL_HEADER.ImageBase + pe.OPTIONAL_HEADER.AddressOfEntryPoint:08X}",
        "linker_version": f"{pe.OPTIONAL_HEADER.MajorLinkerVersion}.{pe.OPTIONAL_HEADER.MinorLinkerVersion:02d}",
        "pdb": {"guid": pdb_guid, "age": codeview.Age},
        "sections": [section.Name.rstrip(b"\0").decode("ascii") for section in pe.sections],
        "imports": [entry.dll.decode("ascii") for entry in pe.DIRECTORY_ENTRY_IMPORT],
        "delay_imports": [entry.dll.decode("ascii") for entry in getattr(pe, "DIRECTORY_ENTRY_DELAY_IMPORT", [])],
    }
    out = ROOT / "analysis" / args.build / "pe.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(metadata, indent=2) + "\n")
    if machine == 0x8664:
        write_pdata(pe, out.parent / "pdata.tsv")


def write_pdata(pe, path):
    """Function bounds from .pdata. A chained entry's unwind info ends with the parent RUNTIME_FUNCTION."""
    base = pe.OPTIONAL_HEADER.ImageBase
    d = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    raw = pe.get_data(d.VirtualAddress, d.Size)
    rows, parts = [], 0
    for i in range(0, len(raw) - 11, 12):
        begin, end, unwind = struct.unpack_from("<III", raw, i)
        if not begin:
            continue
        head = pe.get_data(unwind & ~1, 4)
        flags, codes = head[0] >> 3, head[2]
        part_of = ""
        if flags & 4:  # UNW_FLAG_CHAININFO: the parent's RUNTIME_FUNCTION follows the (even-padded) unwind codes
            parent = pe.get_data((unwind & ~1) + 4 + 2 * (codes + (codes & 1)), 12)
            part_of = f"{base + struct.unpack_from('<I', parent, 0)[0]:08X}"
            parts += 1
        rows.append((base + begin, end - begin, part_of))
    rows.sort()
    with open(path, "w", newline="\n") as f:
        f.write("va\tsize\tpart_of\n")
        for va, size, part_of in rows:
            f.write(f"{va:08X}\t{size}\t{part_of}\n")
    print(f"{path}: {len(rows) - parts:,} functions, {parts:,} chained parts")


if __name__ == "__main__":
    main()
