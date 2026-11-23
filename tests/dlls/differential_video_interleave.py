#!/usr/bin/env python3
"""Original-vs-source differential for Bink video bitmask stream interleaver.

Runs original FUN_1800237B0 at RVA 0x237B0 in the pinned original binkw64.dll
through Unicorn, comparing both its return value and all output bytes to our
source-level C++ reconstruction. Does not use FFmpeg or a replacement codec.
"""
import ctypes
import hashlib
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine  # noqa: E402

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x237B0


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    library_path = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_video_interleave.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original binkw64.dll SHA-256 mismatch")
    image = Image(original)
    emulator = Machine(image)
    library = ctypes.CDLL(str(library_path.resolve()))
    fn = library.bink_video_interleave_bytes
    fn.argtypes = (
        ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint8),
        ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8), ctypes.c_int)
    fn.restype = ctypes.c_int

    rng = random.Random(RVA)
    checks = 0
    for length in (1, 2, 3, 4, 8, 16, 31, 32, 64):
        for attempt in range(50):
            mask = (0, 0xFFFFFFFFFFFFFFFF, 0xAAAAAAAAAAAAAAAA,
                    0x5555555555555555)[attempt] if attempt < 4 else rng.getrandbits(64)
            first = rng.randbytes(length)
            second = rng.randbytes(length)
            output = bytes([0xA5]) * (2 * length)
            expected_return, buffers, _, _ = emulator.call(
                image.base + RVA, [mask, output, first, second, length], "int")
            if isinstance(expected_return, str) and expected_return.startswith("fault:"):
                raise AssertionError(f"Original Bink interleave fault "
                                     f"length={length}, mask={mask:#x}: {expected_return}")
            native_output = (ctypes.c_uint8 * len(output)).from_buffer_copy(output)
            native_first = (ctypes.c_uint8 * len(first)).from_buffer_copy(first)
            native_second = (ctypes.c_uint8 * len(second)).from_buffer_copy(second)
            source_return = fn(mask, native_output, native_first, native_second, length)
            result = ctypes.string_at(ctypes.addressof(native_output), len(output))
            if source_return != expected_return or result != buffers[0]:
                raise AssertionError(
                    f"Bink video interleave mismatch n={length}, mask={mask:#x}: "
                    f"original return={expected_return}, reconstructed={source_return}, "
                    f"original bytes={buffers[0].hex()}, reconstructed={result.hex()}")
            if buffers[1] != first or buffers[2] != second:
                raise AssertionError("Original Bink interleave modified input streams")
            checks += 1
    print(f"Bink stream interleave 0x{RVA:X} differential PASS: {checks} inputs")


if __name__ == "__main__":
    main()
