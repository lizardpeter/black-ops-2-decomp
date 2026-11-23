#!/usr/bin/env python3
"""Fetch hash-pinned Store x64 DLL originals as data, without running them."""
import hashlib
import urllib.parse
import urllib.request
from pathlib import Path
import argparse

BASE = "https://r2.houseofkublai.com/"
PINNED = {
    "XCurl.dll": "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551",
    "binkw64.dll": "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17",
}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dest", nargs="?", type=Path, default=Path("originals/dlls"))
    args = ap.parse_args()
    args.dest.mkdir(parents=True, exist_ok=True)
    for filename, sha in PINNED.items():
        path = args.dest / filename
        if path.exists() and hashlib.sha256(path.read_bytes()).hexdigest() == sha:
            print(f"Already verified: {path}")
            continue
        req = urllib.request.Request(BASE + urllib.parse.quote("bo2/DUMP/" + filename),
                                     headers={"User-Agent": "black-ops-2-decomp-dll-inventory/1"})
        h = hashlib.sha256()
        partial = path.with_suffix(path.suffix + ".partial")
        try:
            with urllib.request.urlopen(req, timeout=45) as remote, partial.open("wb") as local:
                while chunk := remote.read(1024 * 1024):
                    h.update(chunk)
                    local.write(chunk)
            if h.hexdigest() != sha:
                raise ValueError(f"{filename}: SHA-256 mismatch {h.hexdigest()}")
            partial.replace(path)
        finally:
            partial.unlink(missing_ok=True)
        print(f"Verified: {path} ({path.stat().st_size} bytes)")

if __name__ == "__main__":
    main()
