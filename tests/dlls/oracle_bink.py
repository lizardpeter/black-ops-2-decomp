#!/usr/bin/env python3
"""Emulate original pinned Bink x64 leaves and assert instruction-derived behavior.

This checks the ORIGINAL DLL in Unicorn without loading it into a Windows process.
It does not yet compare a rebuilt Windows DLL side by side; the C++ transcriptions
have separately compiled source-level tests. Do not call these 'matches' yet.
"""
import hashlib
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine, OBJECTS  # noqa: E402

SHA256 = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"


def u32(writes, address):
    return struct.unpack("<I", b"".join(writes[address + i] for i in range(4)))[0]


def u64(writes, address):
    return struct.unpack("<Q", b"".join(writes[address + i] for i in range(8)))[0]


def run(path):
    if hashlib.sha256(path.read_bytes()).hexdigest() != SHA256:
        raise SystemExit("Bink original DLL SHA-256 mismatch")
    img = Image(path)
    assert img.x64 and img.base == 0x180000000
    m = Machine(img)
    base = img.base
    checked = 0

    def call(symbol, args, ret="void", setup=None):
        nonlocal checked
        value, buffers, writes, _ = m.call(img.exports[symbol], args, ret, setup=setup)
        if isinstance(value, str) and value.startswith("fault:"):
            raise AssertionError(f"{symbol}: {value}")
        checked += 1
        return value, buffers, writes

    _, _, writes = call("BinkSetIOSize", [2048])
    assert u32(writes, base + 0x570E0) == 2048
    _, _, writes = call("BinkSetMemory", [0x31000080, 0x31000120])
    assert u64(writes, base + 0x542A8) == 0x31000080
    assert u64(writes, base + 0x542B0) == 0x31000120

    _, _, writes = call("BinkSetSoundTrack", [3, struct.pack("<III", 5, 6, 7)])
    assert u32(writes, base + 0x570B0) == 3
    assert [u32(writes, base + 0x57250 + 4*i) for i in range(3)] == [5, 6, 7]

    _, buffers, _ = call("BinkRegisterFrameBuffers", [bytes(0x200), 0x31004000])
    assert struct.unpack_from("<Q", buffers[0], 0xD0)[0] == 0x31004000
    assert call("BinkGetError", [], "cstr")[0] == b""
    _, _, writes = call("BinkSetError", [b"Original Bink\0"])
    error_base = base + 0x31860
    assert b"".join(writes[error_base + i] for i in range(13)) == b"Original Bink"
    _, _, writes = call("BinkSetError", [b"x" * 300 + b"\0"])
    assert b"".join(writes[error_base + i] for i in range(254)) == b"x" * 254
    assert writes[error_base + 254] == b"\0"

    _, _, writes = call("BinkSetFrameRate", [24000, 1001])
    assert u32(writes, base + 0x570E4) == 24000
    assert u32(writes, base + 0x311C0) == 1001
    _, _, writes = call("BinkSetFileOffset", [0x123456789ABCDEFF])
    assert u64(writes, base + 0x57128) == 0x123456789ABCDEFF
    _, _, writes = call("BinkSetSimulate", [77])
    assert u32(writes, base + 0x570DC) == 77

    def handle(flags):
        data = bytearray(0x200)
        struct.pack_into("<I", data, 0x20, flags)
        return bytes(data)

    _, buffers, _ = call("BinkSetWillLoop", [handle(0x1234), 1])
    assert struct.unpack_from("<I", buffers[0], 0x20)[0] == (0x1234 | 0x80)
    _, buffers, _ = call("BinkSetWillLoop", [handle(0x12B4), 0])
    assert struct.unpack_from("<I", buffers[0], 0x20)[0] == (0x12B4 & ~0x80)
    v, buffers, _ = call("BinkSetVideoOnOff", [bytes(0x200), 7], "uint")
    assert v == 7 and struct.unpack_from("<I", buffers[0], 0x134)[0] == 7
    assert call("BinkSetVideoOnOff", [0, 5], "uint")[0] == 5

    def setup_tables(writer):
        writer.write(OBJECTS + 0x100, struct.pack("<III", 30, 40, 50))
        writer.write(OBJECTS + 0x300, struct.pack("<III", 1, 2, 3))

    h = bytearray(0x200)
    struct.pack_into("<Q", h, 0x1D0, OBJECTS + 0x100)
    struct.pack_into("<Q", h, 0x1C8, OBJECTS + 0x300)
    assert call("BinkGetTrackID", [bytes(h), 1], "uint", setup_tables)[0] == 40
    assert call("BinkGetTrackType", [bytes(h), 2], "uint", setup_tables)[0] == 3
    assert call("BinkGetTrackID", [0, 1], "uint")[0] == 0
    assert call("BinkGetTrackType", [0, 1], "uint")[0] == 0
    # RVA 0x17870: metadata for one or two registered YUV frame buffers.
    # This is an imported BO2 function. Test the original's full 216-byte
    # descriptor, including the 96-byte copy for multi-frame files.
    for frame_count in (1, 2):
        handle = bytearray(0x200)
        struct.pack_into("<I", handle, 0x08, frame_count)
        struct.pack_into("<I", handle, 0x20, 0x00100000)
        struct.pack_into("<I", handle, 0x1B8, 1921)
        struct.pack_into("<I", handle, 0x1BC, 1081)
        _, bufs, _ = call("BinkGetFrameBuffersInfo",
                          [bytes(handle), bytes([0xA5]) * (54 * 4)])
        words = struct.unpack("<54I", bufs[1])
        assert words[0] == frame_count
        assert words[1:7] == (1928, 1088, 968, 544, 0, 1)
        assert words[12] == words[18] == words[24] == 1
        for off in (8, 14, 20, 26):
            assert words[off:off + 2] == (0, 0)
        for off in (10, 16, 22, 28):
            assert words[off] == 0
        if frame_count == 1:
            assert words[30] == 0xA5A5A5A5
        else:
            assert words[30:54] == words[6:30]

    print(f"Passed {checked} original-Bink oracle calls (not full DLL differential)")


if __name__ == "__main__":
    path = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    run(path)
