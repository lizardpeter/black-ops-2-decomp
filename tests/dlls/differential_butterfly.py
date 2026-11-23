#!/usr/bin/env python3
"""Bitwise differential test: original Bink SSE butterfly vs C++ reconstruction.

The original code at RVA 0x2ED50 runs under Unicorn with pinned Bink bytes.
The reconstructed transform runs in a locally built test .so. Compare all
eight 32-bit float results as raw bytes; this is NOT full codec verification.
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
RVA = 0x2ED50


def main():
    orig = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    lib_path = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_transform_test.so"
    assert hashlib.sha256(orig.read_bytes()).hexdigest() == PIN
    image = Image(orig)
    assert image.base == 0x180000000
    machine = Machine(image)
    library = ctypes.CDLL(str(lib_path.resolve()))
    library.bink_transform_butterfly8.argtypes = [ctypes.POINTER(ctypes.c_float)]
    library.bink_transform_butterfly8.restype = None

    rng = random.Random(0x2ED50)
    inputs = [[0.0] * 8, [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0],
              [-0.0,0.0,-1.0,1.0,-2.0,2.0,-3.0,3.0],
              [1e-20,-1e-20,1e20,-1e20,3.14,-2.71,1e-10,-1e-10]]
    for _ in range(256):
        inputs.append([rng.uniform(-1e6, 1e6) for _ in range(8)])

    for case, values in enumerate(inputs):
        packed = struct.pack("<8f", *values)
        ret, bufs, _, _ = machine.call(image.base + RVA, [packed], "void")
        if ret is not None:
            raise AssertionError(f"original transform fault case {case}: {ret}")
        src = (ctypes.c_float * 8).from_buffer_copy(packed)
        library.bink_transform_butterfly8(src)
        actual = ctypes.string_at(ctypes.addressof(src), len(packed))
        if actual != bufs[0]:
            expected = struct.unpack("<8I", bufs[0])
            got = struct.unpack("<8I", actual)
            raise AssertionError(f"case {case} mismatched original bits: "
                                 f"original {expected}, reconstruction {got}")
    print(f"Bink 0x2ED50 SSE butterfly differential PASS: {len(inputs)} vectors, 8 floats each")


if __name__ == "__main__":
    main()
