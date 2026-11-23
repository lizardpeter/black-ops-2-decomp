#!/usr/bin/env python3
"""Read-only PE export / import / x64 unwind inventory for BO2 DLL reconstruction.

Requires the repository's existing pefile dependency; it does not load code.
Usage:
  python tools/dlls/fetch.py
  python tools/dlls/inventory.py originals/dlls/XCurl.dll originals/dlls/binkw64.dll
  python tools/dlls/inventory.py originals/dlls/XCurl.dll originals/dlls/binkw64.dll \
      --clients originals/t6mp_msstore.exe originals/t6zm_msstore.exe originals/t6sp_msstore.exe
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

import pefile

PINNED = {
    "xcurl.dll": "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551",
    "binkw64.dll": "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17",
}

def inspect_file(path):
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    pin = PINNED.get(path.name.lower())
    if pin and digest != pin:
        raise ValueError(f"{path}: unexpected SHA-256: {digest}")
    pe = pefile.PE(data=raw, fast_load=False)
    pe.parse_data_directories()
    imports = {
        entry.dll.decode("ascii", "replace"): [
            (item.name.decode("ascii", "replace") if item.name else "#" + str(item.ordinal))
            for item in entry.imports
        ]
        for entry in getattr(pe, "DIRECTORY_ENTRY_IMPORT", [])
    }
    pdata = []
    d = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    if d.VirtualAddress and d.Size and pe.FILE_HEADER.Machine == 0x8664:
        table = pe.get_data(d.VirtualAddress, d.Size)
        for i in range(0, len(table) - 11, 12):
            start, end, unwind = struct.unpack_from("<III", table, i)
            if start and end > start:
                pdata.append((start, end, unwind))
        pdata.sort()
    exports = []
    for entry in getattr(getattr(pe, "DIRECTORY_ENTRY_EXPORT", None), "symbols", []):
        rva = entry.address
        region = next(((a, b) for a, b, _ in pdata if a <= rva < b), None)
        exports.append({
            "name": entry.name.decode("ascii", "replace") if entry.name else None,
            "ordinal": entry.ordinal,
            "rva": f"{rva:08X}",
            "unwind_start": f"{region[0]:08X}" if region else None,
            "unwind_end": f"{region[1]:08X}" if region else None,
            "unwind_size": region[1] - region[0] if region else None,
            "forwarder": entry.forwarder.decode("ascii", "replace") if entry.forwarder else None,
        })
    return {
        "filename": path.name,
        "sha256": digest,
        "bytes": len(raw),
        "image_base": f"{pe.OPTIONAL_HEADER.ImageBase:X}",
        "imports": imports,
        "exports": sorted(exports, key=lambda x: int(x["rva"], 16)),
        "x64_unwind_record_count": len(pdata),
        "note": "Leaves and thunks may lack unwind records; counts are not full function counts.",
    }

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dlls", nargs="+", type=Path)
    ap.add_argument("--clients", nargs="*", type=Path, default=[])
    ap.add_argument("--json", type=Path, help="Optional output path for analysis JSON")
    args = ap.parse_args()
    result = {"dlls": [inspect_file(x) for x in args.dlls], "clients": []}
    for path in args.clients:
        x = inspect_file(path)
        result["clients"].append({
            "filename": path.name,
            "dll_imports": {
                dll: names for dll, names in x["imports"].items()
                if dll.lower() in PINNED
            },
        })
    for x in result["dlls"]:
        print(f"{x['filename']} {x['sha256']}: {len(x['exports'])} exports, "
              f"{x['x64_unwind_record_count']} x64 unwind regions")
        for f in x["exports"]:
            print(f"  {f['rva']} {str(f['name'] or '#'+str(f['ordinal'])):<36} "
                  f"{f['unwind_size'] if f['unwind_size'] is not None else 'leaf/unknown'}")
    for x in result["clients"]:
        print(x["filename"])
        for dll, funcs in x["dll_imports"].items():
            print(f"  {dll}: {len(funcs)} functions")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(result, indent=2) + "\n")

if __name__ == "__main__":
    main()
