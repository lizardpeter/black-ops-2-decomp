#!/usr/bin/env python3
"""Original-x64-versus-source differential for Bink's 8x8 video inverse transform.

The original FUN_18002F2E0 from SHA-256-pinned binkw64.dll is executed in
Unicorn, while a separately compiled C++ reconstruction is called via ctypes.
All 8x8 output pixels and untouched padding bytes are compared bit-for-bit.
This is a real codec block primitive, but NOT an end-to-end Bink video decoder.
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
RVA = 0x2F2E0


def inputs():
    rng = random.Random(RVA)
    yield [0]*64, [2048]*64
    for dc in (1, 128, 1024, -128, -1024, 32767, -32768):
        yield [dc] + [0]*63, [2048]*64
    for _ in range(160):
        coeff = [rng.randint(-4096, 4095) if rng.randrange(5) == 0 else 0
                 for _ in range(64)]
        quant = [rng.randint(-4096, 4095) for _ in range(64)]
        yield coeff, quant
    for _ in range(40):
        coeff = [rng.randint(-64, 64) for _ in range(64)]
        quant = [rng.randint(0, 2048) for _ in range(64)]
        yield coeff, quant


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstructed = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_video_inverse8x8.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("Original Bink hash mismatch")
    img = Image(original)
    m = Machine(img)
    lib = ctypes.CDLL(str(reconstructed.resolve()))
    fn = lib.bink_video_inverse8x8
    fn.argtypes = (
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_int32,
        ctypes.POINTER(ctypes.c_int16), ctypes.POINTER(ctypes.c_int32))
    fn.restype = None

    cases = list(inputs())
    checks = 0
    for i, (coeff, quant) in enumerate(cases):
        c_bytes = struct.pack("<64h", *coeff)
        q_bytes = struct.pack("<64i", *quant)
        for stride in (8, 16, 32):
            output = bytes([0xA5]) * (stride * 8)
            ret, bufs, _, _ = m.call(
                img.base + RVA, [output, stride, c_bytes, q_bytes], "void")
            if ret is not None:
                raise AssertionError(f"Original 0x{RVA:X} fault on case {i}, "
                                     f"stride {stride}: {ret}")
            pixels = (ctypes.c_uint8 * len(output)).from_buffer_copy(output)
            native_coeff = (ctypes.c_int16 * 64).from_buffer_copy(c_bytes)
            native_quant = (ctypes.c_int32 * 64).from_buffer_copy(q_bytes)
            fn(pixels, stride, native_coeff, native_quant)
            actual = ctypes.string_at(ctypes.addressof(pixels), len(output))
            if actual != bufs[0]:
                diff = next(j for j, (x, y) in enumerate(zip(actual, bufs[0])) if x != y)
                raise AssertionError(
                    f"Video inverse mismatch case={i} stride={stride}, byte={diff}: "
                    f"original={bufs[0][diff]:02X} reconstructed={actual[diff]:02X}; "
                    f"original_block={bufs[0].hex()}; reconstructed={actual.hex()}")
            if bufs[1] != c_bytes or bufs[2] != q_bytes:
                raise AssertionError("Original mutated input coefficients or quantization")
            checks += 1
    print(f"Bink video inverse 8x8 RVA 0x{RVA:X} exact differential PASS: "
          f"{checks} blocks across three strides")


if __name__ == "__main__":
    main()
