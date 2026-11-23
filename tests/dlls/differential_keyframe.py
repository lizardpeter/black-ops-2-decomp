#!/usr/bin/env python3
"""Differential original-vs-reconstructed BinkGetKeyFrame.

Unicorn executes the exact SHA256-pinned Store Bink x64 DLL at export
BinkGetKeyFrame, while ctypes executes our separately compiled C++ source.
Both see matching frame-offset arrays, handle fields and mode flags.
Test only valid 1-based frame numbers to avoid intentional original OOB.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine, OBJECTS  # noqa: E402

ORIGINAL_SHA256 = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
ORIGINAL_OFFSET_TABLE = OBJECTS + 0x1000


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    source = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_keyframe_test.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != ORIGINAL_SHA256:
        raise SystemExit("Wrong original DLL")
    image = Image(original)
    m = Machine(image)
    lib = ctypes.CDLL(str(source.resolve()))
    fn = lib.BinkGetKeyFrame
    fn.argtypes = (ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint8)
    fn.restype = ctypes.c_uint32

    rng = random.Random(0x1B2B0)
    checks = 0
    for length in (2, 5, 10, 30, 100):
        for seed in range(12):
            # Index bit 0 identifies a keyframe; all streams have frame 1
            # marked, and the end sentinel has no keyframe bit.
            indices = [100 + i * 100 + int(i == 0 or rng.randrange(5) == 0)
                       for i in range(length)] + [100 + length * 100]
            packed = struct.pack("<" + "I" * len(indices), *indices)
            original_handle = bytearray(0x200)
            struct.pack_into("<I", original_handle, 8, length)
            struct.pack_into("<Q", original_handle, 0x1A0, ORIGINAL_OFFSET_TABLE)

            native_idx = (ctypes.c_uint32 * len(indices))(*indices)
            native_handle = ctypes.create_string_buffer(0x200)
            struct.pack_into("<I", native_handle, 8, length)
            struct.pack_into("<Q", native_handle, 0x1A0, ctypes.addressof(native_idx))

            def setup(writer):
                writer.write(ORIGINAL_OFFSET_TABLE, packed)

            frames = sorted({1, 2, length, max(1, length // 2),
                             min(length, length // 2 + 1)})
            for frame in frames:
                for mode in (0, 1, 2, 3, 0x80, 0x81, 0x82, 0x83, 0xFF):
                    want, _, _, _ = m.call(image.exports["BinkGetKeyFrame"],
                                           [bytes(original_handle), frame, mode],
                                           ret="uint", setup=setup)
                    if isinstance(want, str) and want.startswith("fault:"):
                        raise AssertionError(f"Original fault: {want} "
                                             f"frames={length} frame={frame} mode={mode}")
                    got = fn(native_handle, frame, mode)
                    if got != want:
                        raise AssertionError(f"Mismatch at n={length} seed={seed} "
                                             f"frame={frame} mode={mode}: "
                                             f"original={want}, source={got}, offsets={indices}")
                    checks += 1

    assert fn(None, 1, 0) == 0
    want, _, _, _ = m.call(image.exports["BinkGetKeyFrame"], [0, 1, 0], "uint")
    assert want == 0
    print(f"BinkGetKeyFrame differential PASS: {checks} valid index/mode cases + null")


if __name__ == "__main__":
    main()
