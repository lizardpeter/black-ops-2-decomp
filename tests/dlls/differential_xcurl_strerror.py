#!/usr/bin/env python3
"""Exact-string differential test for original curl_easy_strerror (XCurl.dll).

Original code executes in Unicorn from the pinned x64 PE. Reconstructed C++
runs in a separate native test shared library. Test every error code in the
known range, holes, negative codes, and out-of-range values.
"""
import ctypes
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine  # noqa: E402

SHA = "65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551"


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/XCurl.dll"
    reconstruction = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/xcurl_strerror_test.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA:
        raise SystemExit("XCurl.dll SHA-256 mismatch")

    img = Image(original)
    m = Machine(img)
    lib = ctypes.CDLL(str(reconstruction.resolve()))
    fn = lib.curl_easy_strerror
    fn.argtypes = (ctypes.c_int,)
    fn.restype = ctypes.c_char_p

    errors = [-(1 << 31), -1000, -255, -129] + list(range(-128, 161)) + [1024, (1 << 31) - 1]
    checked = 0
    for e in errors:
        want, _, _, _ = m.call(img.exports["curl_easy_strerror"], [e], "cstr")
        if isinstance(want, str) and want.startswith("fault:"):
            raise AssertionError(f"Original XCurl crashed for {e}: {want}")
        got = fn(e)
        if got != want:
            raise AssertionError(f"XCurl error {e}: original {want!r}, source {got!r}")
        checked += 1
    print(f"XCurl curl_easy_strerror exact-string differential PASS: {checked} error codes")


if __name__ == "__main__":
    main()
