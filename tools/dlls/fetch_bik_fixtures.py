#!/usr/bin/env python3
"""Fetch four SHA-pinned original BO2 .bik fixtures from R2 for offline tests.

The fixtures are read-only original media and MUST NOT be committed.
Each is <1.4 MB. No decoding or execution takes place.
"""
import argparse
import hashlib
import urllib.parse
import urllib.request
from pathlib import Path

BASE = "https://r2.houseofkublai.com/"
FIXTURES = {
    "haiti_int_1c.bik": "d74398662e7829a379806104b233ca9161761ae31792675ad22d56627122f85e",
    "la_pip_seq_6.bik": "c1171207707c3cdcc44160899efbdc7016b1df94b21d11c592546983dd15049c",
    "karma_pip_security_feed.bik": "18daa35802784540f6713c083ae78bd6e14bf442f96028201ede851f643db062",
    "yemen_drone_cam.bik": "9fa2f650c715734c69e465352e162951c5c38cb162f99dbd4843115e4cfdd20e",
}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dest", type=Path, nargs="?", default=Path("originals/bik"))
    args = ap.parse_args()
    args.dest.mkdir(parents=True, exist_ok=True)
    for name, sha in FIXTURES.items():
        target = args.dest / name
        if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == sha:
            print("Verified existing", target)
            continue
        url = BASE + urllib.parse.quote("bo2/DUMP/video/" + name)
        request = urllib.request.Request(url, headers={"User-Agent": "black-ops-2-decomp-bik/1"})
        partial = target.with_suffix(".partial")
        digest = hashlib.sha256()
        try:
            with urllib.request.urlopen(request, timeout=45) as remote, partial.open("wb") as out:
                while chunk := remote.read(1 << 20):
                    out.write(chunk)
                    digest.update(chunk)
            if digest.hexdigest() != sha:
                raise ValueError(f"{name}: SHA-256 mismatch: {digest.hexdigest()}")
            partial.replace(target)
        finally:
            partial.unlink(missing_ok=True)
        print("Verified", target)


if __name__ == "__main__":
    main()
