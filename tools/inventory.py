#!/usr/bin/env python3
"""Write a PE inventory for one hash-pinned original without loading it."""
import argparse
import hashlib
import json
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
    if pe.FILE_HEADER.Machine != 0x14c or pe.OPTIONAL_HEADER.ImageBase != build["image_base"]:
        raise ValueError(f"{args.build}: expected x86 PE at the pinned image base")

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


if __name__ == "__main__":
    main()
