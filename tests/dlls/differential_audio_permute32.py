#!/usr/bin/env python3
"""Byte-exact original-versus-source tests for two Bink sample permutations.

Original functions 0x18002A450 and 0x18002A570 execute under the repository's
Unicorn x64 harness. The reconstructed C++ executes in an independent native
shared library. Compares all 128 output bytes, including unusual float32 bit
patterns such as signed zero, NaN, and infinities.

No decoder implementations or codec libraries are reused.
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

SHA256 = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
ORIGINAL_FUNCTIONS = (
    (0x2A450, "bink_audio_permute32_a"),
    (0x2A570, "bink_audio_permute32_b"),
)


def vectors():
    yield list(range(32))
    yield [0] * 32
    yield [0xFFFFFFFF] * 32
    yield [0x7F800000 if n % 2 else 0xFF800000 for n in range(32)]
    yield [0x7FC00000 + n for n in range(32)]
    yield [0x80000000 if n % 2 else 0 for n in range(32)]
    rng = random.Random(0x2A450)
    for _ in range(512):
        yield [rng.getrandbits(32) for _ in range(32)]


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstructed = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_audio_permute32.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("Original binkw64.dll hash mismatch")
    image = Image(original)
    assert image.x64 and image.base == 0x180000000
    emulator = Machine(image)
    library = ctypes.CDLL(str(reconstructed.resolve()))
    samples = list(vectors())
    for rva, name in ORIGINAL_FUNCTIONS:
        func = getattr(library, name)
        func.argtypes = (ctypes.POINTER(ctypes.c_uint32),)
        func.restype = None
        for n, words in enumerate(samples):
            inp = struct.pack("<32I", *words)
            value, bufs, _, _ = emulator.call(image.base + rva, [inp], "void")
            if value is not None:
                raise AssertionError(f"{name}: original fault input {n}: {value}")
            native = (ctypes.c_uint32 * 32)(*words)
            func(native)
            observed = ctypes.string_at(ctypes.addressof(native), 128)
            if observed != bufs[0]:
                raise AssertionError(
                    f"{name} input {n}: original {bufs[0].hex()}, C++ {observed.hex()}")
        print(f"{name} original-vs-source PASS: {len(samples)} 128-byte inputs")


if __name__ == "__main__":
    main()
