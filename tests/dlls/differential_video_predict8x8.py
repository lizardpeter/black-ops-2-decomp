#!/usr/bin/env python3
"""Differential Bink video residual/prediction math, original vs source.

The pinned DLL's original FUN_18002F9F0 selects a quantization matrix from
BINKCONS RVA 0x5C120 + quant_index*0x100. Here that EXACT original matrix is
read from the pinned PE image and passed explicitly to our reconstructed core.
The core implements the original inverse-transform and byte-add arithmetic,
but does NOT yet recreate the complete fixed-table/index ABI boundary.

Original code runs in Unicorn; the decompiled native C++ core is run via ctypes.
No third-party decoder is used, and full Bink frame decoding is not claimed.
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
RVA = 0x2F9F0
QUANT_TABLE_RVA = 0x5C120


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    source = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_video_predict8x8.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Bink original SHA-256 mismatch")
    img = Image(original)
    machine = Machine(img)
    lib = ctypes.CDLL(str(source.resolve()))
    fn = lib.bink_video_predict8x8_core
    fn.argtypes = (
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_int32,
        ctypes.POINTER(ctypes.c_int16), ctypes.POINTER(ctypes.c_int32),
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_int32)
    fn.restype = None

    rng = random.Random(RVA)
    checks = 0
    for quant_index in (0, 1, 3, 5, 8, 11):
        quant_offset = QUANT_TABLE_RVA + quant_index * 256
        table = bytes(img.data[quant_offset:quant_offset + 256])
        if len(table) != 256:
            raise AssertionError("Original quantization table is truncated")
        for sample in range(35):
            coefficients = ([0] * 64 if sample == 0 else
                            [rng.randint(-3000, 3000) if rng.randrange(4) == 0 else 0
                             for _ in range(64)])
            coefficients_bytes = struct.pack("<64h", *coefficients)
            for stride, pred_stride in ((8, 8), (16, 8), (24, 16)):
                initial = bytes([0xA5]) * (stride * 8)
                reference = rng.randbytes(pred_stride * 8)
                want, buffers, _, _ = machine.call(
                    img.base + RVA,
                    [initial, stride, coefficients_bytes, quant_index, reference, pred_stride],
                    "void")
                if want is not None:
                    raise AssertionError(f"Original 0x{RVA:X} fault "
                                         f"index={quant_index} sample={sample}: {want}")
                output = (ctypes.c_uint8 * len(initial)).from_buffer_copy(initial)
                native_coeff = (ctypes.c_int16 * 64).from_buffer_copy(coefficients_bytes)
                native_quant = (ctypes.c_int32 * 64).from_buffer_copy(table)
                native_reference = (ctypes.c_uint8 * len(reference)).from_buffer_copy(reference)
                fn(output, stride, native_coeff, native_quant, native_reference, pred_stride)
                actual = ctypes.string_at(ctypes.addressof(output), len(initial))
                if actual != buffers[0]:
                    position = next(i for i, (a, b) in enumerate(zip(actual, buffers[0])) if a != b)
                    raise AssertionError(
                        f"Bink video prediction mismatch quant={quant_index}, sample={sample}, "
                        f"stride={stride}, reference stride={pred_stride}, output byte={position}: "
                        f"original={buffers[0][position]}, reconstructed={actual[position]}")
                if buffers[2] != reference or buffers[1] != coefficients_bytes:
                    raise AssertionError("Original Bink prediction modified its input buffers")
                checks += 1
    print(f"Bink predicted 8x8 core RVA 0x{RVA:X} differential PASS: {checks} outputs "
          "(original quantization matrices passed explicitly)")


if __name__ == "__main__":
    main()
