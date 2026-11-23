#!/usr/bin/env python3
"""Differential original-vs-reconstructed BinkGetFrameBuffersInfo test.

The original SHA-256-pinned Bink x64 DLL runs under Unicorn. The reconstructed
source runs as a Linux test .so compiled from bink_frames.cpp. Identical
input structures and sentinel-initialized descriptor buffers are compared
byte-for-byte, including bytes the original intentionally leaves unchanged.
This verifies ONLY this one export, not Bink video decoding.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine  # noqa: E402

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"


def make_handle(frames, width, height, flags):
    handle = bytearray(0x200)
    struct.pack_into("<I", handle, 0x08, frames)
    struct.pack_into("<I", handle, 0x20, flags)
    struct.pack_into("<I", handle, 0x1B8, width)
    struct.pack_into("<I", handle, 0x1BC, height)
    return bytes(handle)


def main():
    orig = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    recon = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_frames_test.so"
    if hashlib.sha256(orig.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original Bink SHA-256 mismatch")
    image = Image(orig)
    machine = Machine(image)
    lib = ctypes.CDLL(str(recon.resolve()))
    lib.BinkGetFrameBuffersInfo.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    lib.BinkGetFrameBuffersInfo.restype = None

    rng = random.Random(0xB12B1)
    cases = [(1, 1920, 1080, 0), (2, 1921, 1081, 0x100000),
             (0, 0, 0, 0), (3, 1, 1, 0), (1, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFFF)]
    cases += [(rng.choice([0, 1, 2, 3, 30]), rng.getrandbits(32),
               rng.getrandbits(32), rng.getrandbits(32)) for _ in range(70)]

    checks = 0
    for frames, width, height, flags in cases:
        original_handle = make_handle(frames, width, height, flags)
        for fill in (0, 0xA5, 0xFF):
            original_out = bytes([fill]) * (54 * 4)
            result, original_buffers, _, _ = machine.call(
                image.exports["BinkGetFrameBuffersInfo"],
                [original_handle, original_out], "void")
            if isinstance(result, str) and result.startswith("fault:"):
                raise AssertionError((frames, width, height, flags, result))
            src_h = ctypes.create_string_buffer(original_handle, len(original_handle))
            src_o = ctypes.create_string_buffer(original_out, len(original_out))
            lib.BinkGetFrameBuffersInfo(src_h, src_o)
            if bytes(src_o) != original_buffers[1]:
                diffs = [i for i, (a, b) in enumerate(zip(original_buffers[1], bytes(src_o)))
                         if a != b]
                raise AssertionError(f"BinkGetFrameBuffersInfo mismatch: "
                                     f"{(frames, width, height, flags, fill)!r}; "
                                     f"first differing byte offsets: {diffs[:16]}")
            if bytes(src_h) != original_buffers[0]:
                raise AssertionError("Input handle mutated differently")
            checks += 1
    # Null args: no writes, including when output pointer is valid.
    value, original_buffers, _, _ = machine.call(image.exports["BinkGetFrameBuffersInfo"],
                                                   [0, b"Q" * 216], "void")
    assert value is None and original_buffers[0] == b"Q" * 216
    src_o = ctypes.create_string_buffer(b"Q" * 216, 216)
    lib.BinkGetFrameBuffersInfo(None, src_o)
    assert bytes(src_o) == b"Q" * 216
    print(f"BinkGetFrameBuffersInfo differential PASS: {checks} data cases + null case")


if __name__ == "__main__":
    main()
