#!/usr/bin/env python3
"""Verify the standard reflected CRC32 table in hash-pinned local Store PE files.

Read-only: no game process, binary modification, or service access.
"""
import argparse
import hashlib
from pathlib import Path
import struct

PINS = {
    "t6mp.exe": "cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01",
    "t6zm.exe": "7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416",
    "t6sp.exe": "0404f79cb36394b14cfc3321b7aa126a08518531564fa042fc14b82debe45c89",
}


def crc_table():
    words = []
    for seed in range(256):
        crc = seed
        for _ in range(8):
            crc = (crc >> 1) ^ (0xEDB88320 if crc & 1 else 0)
        words.append(crc)
    return struct.pack("<256I", *words)


def data_sections(data):
    if data[:2] != b"MZ":
        raise ValueError("missing DOS header")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("missing PE header")
    machine, count = struct.unpack_from("<HH", data, pe + 4)
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    if machine != 0x8664 or struct.unpack_from("<H", data, pe + 24)[0] != 0x20B:
        raise ValueError("not an x64 PE32+ image")
    first = pe + 24 + optional_size
    if first + count * 40 > len(data):
        raise ValueError("truncated section headers")
    for i in range(count):
        off = first + i * 40
        name = data[off:off + 8].split(b"\0", 1)[0]
        size, raw = struct.unpack_from("<II", data, off + 16)
        if raw + size > len(data):
            raise ValueError("truncated section data")
        if name in (b".rdata", b".data", b".rodata", b"_RDATA"):
            yield data[raw:raw + size]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="local directory with all three Store executables")
    args = parser.parse_args()
    expected = crc_table()
    for name, pin in PINS.items():
        data = (args.directory / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != pin:
            raise SystemExit(f"{name}: SHA-256 mismatch")
        matches = sum(section.count(expected) for section in data_sections(data))
        if matches != 1:
            raise SystemExit(f"{name}: expected one complete CRC32 table, got {matches}")
        print(f"{name}: pinned x64 image; all 256 CRC32 entries match")


if __name__ == "__main__":
    main()
