#!/usr/bin/env python3
"""Differential test of original Bink 0x2EE30 and 0x2EF50 audio rotations.

Run exact pinned Bink x64 instructions in Unicorn vs native C++ source.
For valid sizes used by the codec, compare ALL output float bits and ensure
coefficients have not changed. This does not imply whole-decoder coverage.
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
KERNELS = ((0x2EE30, "bink_transform_rotate_stage"),
           (0x2EF50, "bink_transform_rotate_stage_alt"))


def main():
    orig = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    source = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_rotate_test.so"
    if hashlib.sha256(orig.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Bink original DLL SHA-256 mismatch")
    image = Image(orig)
    machine = Machine(image)
    library = ctypes.CDLL(str(source.resolve()))
    for _, name in KERNELS:
        fn = getattr(library, name)
        fn.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_float),
                       ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
        fn.restype = None

    rng = random.Random(0x2EE30)
    checks = 0
    for rva, name in KERNELS:
        fn = getattr(library, name)
        checks = 0
        for count in (8, 16, 32, 64):
            for size in (32, 64, 128):
                for index in range(32):
                    samples = [rng.uniform(-100.0, 100.0) for _ in range(count)]
                    coeff = [rng.uniform(-0.4, 0.9) for _ in range(size + 1)]
                    if index == 0:
                        samples = [0.0] * count
                    if index == 1:
                        coeff = [0.5] * (size + 1)
                    packed = struct.pack(f"<{count}f", *samples)
                    table = struct.pack(f"<{size+1}f", *coeff)
                    value, bufs, _, _ = machine.call(
                        image.base + rva, [count, packed, size, table], "void")
                    if value is not None:
                        raise AssertionError(f"Original 0x{rva:X} fault "
                                             f"count={count} size={size}: {value}")
                    native_samples = (ctypes.c_float * count).from_buffer_copy(packed)
                    native_coeff = (ctypes.c_float * (size + 1)).from_buffer_copy(table)
                    fn(count, native_samples, size, native_coeff)
                    actual = ctypes.string_at(ctypes.addressof(native_samples), len(packed))
                    if actual != bufs[0]:
                        a = struct.unpack(f"<{count}I", actual)
                        b = struct.unpack(f"<{count}I", bufs[0])
                        raise AssertionError(f"0x{rva:X}: count={count} size={size} "
                                             f"case={index} original={b}, actual={a}")
                    if ctypes.string_at(ctypes.addressof(native_coeff), len(table)) != bufs[1]:
                        raise AssertionError("Coefficient table changed unexpectedly")
                    checks += 1
        print(f"Bink 0x{rva:X} audio rotate differential PASS: {checks} inputs")


if __name__ == "__main__":
    main()
