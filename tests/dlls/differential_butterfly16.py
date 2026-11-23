#!/usr/bin/env python3
"""Bit-for-bit differential test of original Bink FUN_18002E630 16-float core.

Pinned original x64 instructions run under Unicorn; the separately compiled
reconstruction runs under ctypes. Compares all output bytes for a range of
finite inputs and coefficients, with no external media decoder library.
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
RVA = 0x2E630


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstructed = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_transform16_test.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("Bink original hash mismatch")
    img = Image(original)
    m = Machine(img)
    lib = ctypes.CDLL(str(reconstructed.resolve()))
    fn = lib.bink_transform_butterfly16
    fn.argtypes = (ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float))
    fn.restype = None

    rng = random.Random(RVA)
    checks = 0
    fixed = [[0.0] * 16, [float(x) for x in range(16)],
             [(-1.0) ** x * (x + 1.0) for x in range(16)]]
    fixed += [[rng.uniform(-500, 500) for _ in range(16)] for _ in range(256)]

    for case, values in enumerate(fixed):
        for coeff in (0.0, 0.5, 0.70710678118, -0.7):
            input_bytes = struct.pack("<16f", *values)
            coefficient_bytes = struct.pack("<2f", 1.25, coeff)
            want, buffers, _, _ = m.call(img.base + RVA, [input_bytes, coefficient_bytes], "void")
            if want is not None:
                raise AssertionError(f"Original fault case {case}: {want}")
            native_in = (ctypes.c_float * 16).from_buffer_copy(input_bytes)
            native_coef = (ctypes.c_float * 2).from_buffer_copy(coefficient_bytes)
            fn(native_in, native_coef)
            got = ctypes.string_at(ctypes.addressof(native_in), len(input_bytes))
            if got != buffers[0]:
                for i, (a, b) in enumerate(zip(struct.unpack("<16I", got),
                                               struct.unpack("<16I", buffers[0]))):
                    if a != b:
                        raise AssertionError(f"case={case} coeff={coeff} float={i}: "
                                             f"original bits={b:#010x} source bits={a:#010x}")
            if ctypes.string_at(ctypes.addressof(native_coef), len(coefficient_bytes)) != buffers[1]:
                raise AssertionError("Coefficient pointer modified")
            checks += 1
    print(f"Bink 16-float transform 0x{RVA:X} differential PASS: {checks} vectors")


if __name__ == "__main__":
    main()
