#!/usr/bin/env python3
"""Bit-exact differential test of original Bink 2x inverse 8x8 video block.

Original binkw64.dll FUN_18002F630 runs in Unicorn; our reconstructed
bink_video_inverse8x8_2x and already-verified inverse kernel run natively.
Compares all 16x16 output pixels AND unchanged stride padding. This test does
not imply whole-frame Bink playback or removal of the original DLL.
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
RVA = 0x2F630


def coefficient_cases():
    rng = random.Random(0x2F630)
    yield [0] * 64, [2048] * 64
    for dc in (1, 128, 1024, -128, -1024, 32767, -32768):
        yield [dc] + [0] * 63, [2048] * 64
    for _ in range(112):
        coefficients = [rng.randint(-4096, 4095) if rng.randrange(5) == 0 else 0
                        for _ in range(64)]
        quant = [rng.randint(-4096, 4095) for _ in range(64)]
        yield coefficients, quant
    for _ in range(32):
        yield [rng.randint(-64, 64) for _ in range(64)], [
            rng.randint(0, 2048) for _ in range(64)]


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstructed = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_inverse8x8_2x.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original Bink DLL SHA-256 mismatch")
    image = Image(original)
    assert image.x64 and image.base == 0x180000000
    emulator = Machine(image)
    library = ctypes.CDLL(str(reconstructed.resolve()))
    fn = library.bink_video_inverse8x8_2x
    fn.argtypes = (
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_int32,
        ctypes.POINTER(ctypes.c_int16), ctypes.POINTER(ctypes.c_int32))
    fn.restype = None

    count = 0
    for index, (coeff, quant) in enumerate(coefficient_cases()):
        coefficients = struct.pack("<64h", *coeff)
        quantization = struct.pack("<64i", *quant)
        for stride in (16, 24, 32):
            initial_output = bytes([0xA5]) * (stride * 16)
            result, buffers, _, _ = emulator.call(
                image.base + RVA, [initial_output, stride, coefficients, quantization], "void")
            if result is not None:
                raise AssertionError(f"Original Bink 0x{RVA:X} fault at case={index}, "
                                     f"stride={stride}: {result}")
            output = (ctypes.c_uint8 * len(initial_output)).from_buffer_copy(initial_output)
            native_coeff = (ctypes.c_int16 * 64).from_buffer_copy(coefficients)
            native_quant = (ctypes.c_int32 * 64).from_buffer_copy(quantization)
            fn(output, stride, native_coeff, native_quant)
            actual = ctypes.string_at(ctypes.addressof(output), len(initial_output))
            expected = buffers[0]
            if actual != expected:
                offset = next(i for i, (a, b) in enumerate(zip(actual, expected)) if a != b)
                raise AssertionError(
                    f"Bink 2x inverse mismatch at case={index}, stride={stride}, "
                    f"byte={offset}: original={expected[offset]:#04x}, C++={actual[offset]:#04x}")
            if buffers[1] != coefficients or buffers[2] != quantization:
                raise AssertionError("Original transform modified input tables")
            count += 1
    print(f"Bink 2x inverse 8x8 RVA 0x{RVA:X} differential PASS: "
          f"{count} 16x16 block/stride comparisons")


if __name__ == "__main__":
    main()
