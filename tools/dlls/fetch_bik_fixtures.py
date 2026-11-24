#!/usr/bin/env python3
"""Fetch SHA-pinned original BO2 .bik fixtures from R2 for offline tests.

The fixtures are read-only original media and MUST NOT be committed.
No decoding or execution takes place.
"""
import argparse
import hashlib
import urllib.parse
import urllib.request
from pathlib import Path

BASE = "https://r2.houseofkublai.com/"
FIXTURES = {
    "blackout_virus.bik": "ac4501f8c4c11efb7f39fc3bdd045e834ca2f6a77b55276da6f8a7240e70d72f",
    "blackout_lockout.bik": "9e940b38bc41f8001cc5d752c3cb8f6b8a37e989e3968023014aa228b79b0417",
    "karma_pip_helipad.bik": "a01145cf7264b212f0f96813e28958605ca0a4fced2c5a1579a77b5759d71910",
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
