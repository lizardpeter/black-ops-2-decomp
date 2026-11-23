#!/usr/bin/env python3
"""Differential-test original Bink FUN_180024980 byte Huffman decoder.

Original code from SHA-pinned Store binkw64.dll is executed in Unicorn.
Our C++ reconstruction runs as a separate native shared library.
Compare decoded bytes, buffer cursor/limit, and bit reservoir state.

Synthetic streams use original state layouts, LSB-first packed words, both
fixed and mixed-length (1/2/3-bit) prefix-free Huffman tables, and the
repeated-literal branch. This is not yet full game-frame integration.
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

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x24980
WORDS = OBJECTS + 0x1000
LOOKUP = OBJECTS + 0x2000
OUTPUT = OBJECTS + 0x3000
LENGTH = 128


class Decoder(ctypes.Structure):
    _fields_ = [
        ("current", ctypes.POINTER(ctypes.c_uint8)),
        ("end", ctypes.POINTER(ctypes.c_uint8)),
        ("reserved10", ctypes.c_uint64),
        ("symbol_map", ctypes.c_uint8 * 16),
        ("lookup_bits", ctypes.c_uint32),
        ("reserved2c", ctypes.c_uint32),
        ("lookup_table", ctypes.POINTER(ctypes.c_uint8)),
        ("count_bits", ctypes.c_uint32),
        ("capacity", ctypes.c_uint32),
        ("buffer", ctypes.POINTER(ctypes.c_uint8)),
    ]


class BitStream(ctypes.Structure):
    _fields_ = [
        ("cache", ctypes.c_uint32),
        ("reserved4", ctypes.c_uint32),
        ("next_word", ctypes.POINTER(ctypes.c_uint32)),
        ("available_bits", ctypes.c_uint32),
        ("reserved14", ctypes.c_uint32),
    ]


assert ctypes.sizeof(Decoder) == 0x48
assert ctypes.sizeof(BitStream) == 24


VARIABLE_CODES = ((0, 1), (1, 2), (3, 3), (7, 3))
FIXED_TABLE = bytes(0x40 | i for i in range(16))
VARIABLE_TABLE = bytes(
    (0x10 | 0) if not (i & 1) else
    (0x20 | 1) if (i & 3) == 1 else
    (0x30 | 2) if (i & 7) == 3 else
    (0x30 | 3)
    for i in range(16)
)


def make_stream(count, mode, codes, variable):
    bits = []
    def add(v, n):
        for i in range(n):
            bits.append((v >> i) & 1)
    add(count, 6)
    if count:
        add(mode, 1)
        if mode:
            add(codes[0], 4)
        else:
            for code in codes:
                value, width = VARIABLE_CODES[code] if variable else (code, 4)
                add(value, width)
    words = [0] * 32
    for i, bit in enumerate(bits):
        words[i // 32] |= bit << (i % 32)
    return words


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstruction = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_entropy_bytes.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original binkw64.dll SHA-256 mismatch")
    image = Image(original)
    machine = Machine(image)
    library = ctypes.CDLL(str(reconstruction.resolve()))
    fn = library.bink_decode_entropy_bytes
    fn.argtypes = (ctypes.POINTER(Decoder), ctypes.POINTER(BitStream),
                   ctypes.POINTER(ctypes.c_uint32))
    fn.restype = None

    rng = random.Random(RVA)
    cases = []
    for count in (0, 1, 2, 3, 7, 8, 9, 15, 16, 31, 32, 47, 63):
        for mode in ((0, 1) if count else (0,)):
            for variable in ((False, True) if (count and not mode) else (False,)):
                for _ in range(12):
                    codes = [rng.randrange(4 if variable else 16)
                             for _ in range(count if not mode else 1)]
                    cases.append((count, mode, codes, variable))

    symbols = bytes(range(0x70, 0x80))
    compared = 0
    for case, (count, mode, codes, variable) in enumerate(cases):
        table = VARIABLE_TABLE if variable else FIXED_TABLE
        words = make_stream(count, mode, codes, variable)
        word_bytes = struct.pack("<32I", *words)
        initial = bytes([0xA5]) * LENGTH

        decoder = bytearray(0x48)
        struct.pack_into("<QQQ", decoder, 0, OUTPUT, OUTPUT, 0)
        decoder[0x18:0x28] = symbols
        struct.pack_into("<I", decoder, 0x28, 4)
        struct.pack_into("<Q", decoder, 0x30, LOOKUP)
        struct.pack_into("<IIQ", decoder, 0x38, 6, 63, OUTPUT)
        bitreader = bytearray(24)
        struct.pack_into("<Q", bitreader, 8, WORDS)

        def setup(writer):
            writer.write(WORDS, word_bytes)
            writer.write(LOOKUP, table)
            writer.write(OUTPUT, initial)

        result, buffers, writes, _ = machine.call(
            image.base + RVA, [bytes(decoder), bytes(bitreader), WORDS + len(word_bytes)],
            "void", setup=setup)
        if result is not None:
            raise AssertionError(f"Original Huffman decoder fault case {case}: {result}")
        original_output = b"".join(writes.get(OUTPUT + i, initial[i:i + 1])
                                   for i in range(LENGTH))

        native_output = (ctypes.c_uint8 * LENGTH).from_buffer_copy(initial)
        native_table = (ctypes.c_uint8 * len(table)).from_buffer_copy(table)
        native_symbols = (ctypes.c_uint8 * len(symbols)).from_buffer_copy(symbols)
        native_words = (ctypes.c_uint32 * len(words))(*words)
        native = Decoder()
        native.current = ctypes.cast(native_output, ctypes.POINTER(ctypes.c_uint8))
        native.end = ctypes.cast(native_output, ctypes.POINTER(ctypes.c_uint8))
        native.symbol_map[:] = native_symbols[:]
        native.lookup_bits = 4
        native.lookup_table = ctypes.cast(native_table, ctypes.POINTER(ctypes.c_uint8))
        native.count_bits = 6
        native.capacity = 63
        native.buffer = ctypes.cast(native_output, ctypes.POINTER(ctypes.c_uint8))
        state = BitStream()
        state.next_word = ctypes.cast(native_words, ctypes.POINTER(ctypes.c_uint32))
        end_pointer = ctypes.cast(ctypes.byref(native_words, len(word_bytes)),
                                  ctypes.POINTER(ctypes.c_uint32))
        fn(ctypes.byref(native), ctypes.byref(state), end_pointer)

        source_output = ctypes.string_at(ctypes.addressof(native_output), LENGTH)
        if original_output != source_output:
            i = next(i for i, (a, b) in enumerate(zip(original_output, source_output)) if a != b)
            raise AssertionError(f"Entropy output mismatch case={case} len={count} "
                                 f"mode={mode} variable={variable} at byte {i}: original={original_output[i]} "
                                 f"source={source_output[i]}")
        orig_decoder = buffers[0]
        original_current, original_limit = struct.unpack_from("<QQ", orig_decoder)
        native_base = ctypes.addressof(native_output)
        native_current = ctypes.cast(native.current, ctypes.c_void_p).value
        native_limit = ctypes.cast(native.end, ctypes.c_void_p).value
        if (original_current - OUTPUT, original_limit - OUTPUT) != (
                native_current - native_base, native_limit - native_base):
            raise AssertionError(f"Decoder pointer state mismatch case={case}")
        orig_bits = buffers[1]
        cached, = struct.unpack_from("<I", orig_bits, 0)
        next_pointer, = struct.unpack_from("<Q", orig_bits, 8)
        bits_available, = struct.unpack_from("<I", orig_bits, 16)
        native_next = ctypes.cast(state.next_word, ctypes.c_void_p).value
        if ((cached, (next_pointer - WORDS) // 4, bits_available) !=
                (state.cache, (native_next - ctypes.addressof(native_words)) // 4,
                 state.available_bits)):
            raise AssertionError(
                f"Bit state mismatch case={case} count={count} mode={mode} variable={variable}: "
                f"orig={(cached, next_pointer - WORDS, bits_available)} "
                f"native={(state.cache, native_next - ctypes.addressof(native_words), state.available_bits)}")
        compared += 1

    print(f"Bink original 0x{RVA:X} Huffman byte decoder differential PASS: "
          f"{compared} synthetic streams (mixed-length Huffman and repeat branches)")


if __name__ == "__main__":
    main()
