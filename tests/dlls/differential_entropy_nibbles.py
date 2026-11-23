#!/usr/bin/env python3
"""Differential original-vs-source Bink nibble-pair Huffman decoder (0x24B80).

The original pinned Bink x64 function executes in Unicorn; the reconstructed
C++ routine executes in a separate native library. Synthetic valid LSB-first
bitstreams cover both fixed and mixed 1/2/3-bit Huffman code lengths, including
32-bit reservoir boundaries, output bytes, consumed word pointers, and state.
No third-party codec is used. Whole-frame playback remains a separate goal.
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
from differential_entropy_bytes import (
    Decoder, BitStream, FIXED_TABLE, VARIABLE_TABLE, VARIABLE_CODES,
)

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x24B80
WORDS, LOOKUP, OUTPUT = OBJECTS + 0x1000, OBJECTS + 0x2000, OBJECTS + 0x3000
LENGTH = 128


def pack_stream(count, codes, variable):
    stream_bits = []
    def add(value, width):
        stream_bits.extend((value >> i) & 1 for i in range(width))
    add(count, 6)
    for code in codes:
        value, width = VARIABLE_CODES[code] if variable else (code, 4)
        add(value, width)
    words = [0] * 32
    for i, bit in enumerate(stream_bits):
        words[i // 32] |= bit << (i % 32)
    return words


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstructed = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_entropy_nibbles.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Wrong original binkw64.dll")
    image = Image(original)
    emulator = Machine(image)
    library = ctypes.CDLL(str(reconstructed.resolve()))
    fn = library.bink_decode_entropy_nibbles
    fn.argtypes = (ctypes.POINTER(Decoder), ctypes.POINTER(BitStream),
                   ctypes.POINTER(ctypes.c_uint32))
    fn.restype = None

    rng = random.Random(RVA)
    samples = []
    for count in (0, 1, 2, 3, 7, 8, 9, 15, 16, 31, 32, 47, 63):
        for variable in ((False, True) if count else (False,)):
            for _ in range(12):
                codes = [rng.randrange(4 if variable else 16)
                         for _ in range(count * 2)]
                samples.append((count, codes, variable))

    initial = bytes([0xA5]) * LENGTH
    symbols = bytes(range(16))
    checked = 0
    for case, (count, codes, variable) in enumerate(samples):
        table = VARIABLE_TABLE if variable else FIXED_TABLE
        words = pack_stream(count, codes, variable)
        word_bytes = struct.pack("<32I", *words)
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

        value, bufs, writes, _ = emulator.call(
            image.base + RVA, [bytes(decoder), bytes(bitreader), WORDS + len(word_bytes)],
            "void", setup=setup)
        if value is not None:
            raise AssertionError(f"Original nibble decoder fault case {case}: {value}")
        original_output = b"".join(
            writes.get(OUTPUT+i, initial[i:i+1]) for i in range(LENGTH))

        native_out = (ctypes.c_uint8 * LENGTH).from_buffer_copy(initial)
        native_table = (ctypes.c_uint8 * 16).from_buffer_copy(table)
        native_words = (ctypes.c_uint32 * len(words))(*words)
        state = Decoder()
        state.current = ctypes.cast(native_out, ctypes.POINTER(ctypes.c_uint8))
        state.end = ctypes.cast(native_out, ctypes.POINTER(ctypes.c_uint8))
        state.symbol_map[:] = list(symbols)
        state.lookup_bits = 4
        state.lookup_table = ctypes.cast(native_table, ctypes.POINTER(ctypes.c_uint8))
        state.count_bits = 6
        state.capacity = 63
        state.buffer = ctypes.cast(native_out, ctypes.POINTER(ctypes.c_uint8))
        bitstate = BitStream()
        bitstate.next_word = ctypes.cast(native_words, ctypes.POINTER(ctypes.c_uint32))
        end_ptr = ctypes.cast(ctypes.byref(native_words, len(word_bytes)),
                              ctypes.POINTER(ctypes.c_uint32))
        fn(ctypes.byref(state), ctypes.byref(bitstate), end_ptr)

        actual = ctypes.string_at(ctypes.addressof(native_out), LENGTH)
        if actual != original_output:
            p = next(i for i, (a, b) in enumerate(zip(actual, original_output)) if a != b)
            raise AssertionError(
                f"Nibble entropy mismatch case={case} variable={variable} count={count} "
                f"at byte {p}: original={original_output[p]}, source={actual[p]}")
        oc, oe = struct.unpack_from("<QQ", bufs[0])
        base = ctypes.addressof(native_out)
        nc = ctypes.cast(state.current, ctypes.c_void_p).value
        ne = ctypes.cast(state.end, ctypes.c_void_p).value
        if (oc-OUTPUT, oe-OUTPUT) != (nc-base, ne-base):
            raise AssertionError(f"Decoded pointer state differs at case {case}")
        orig_cache, = struct.unpack_from("<I", bufs[1], 0)
        orig_next, = struct.unpack_from("<Q", bufs[1], 8)
        orig_available, = struct.unpack_from("<I", bufs[1], 16)
        native_next = ctypes.cast(bitstate.next_word, ctypes.c_void_p).value
        if (orig_cache, (orig_next-WORDS)//4, orig_available) != (
                bitstate.cache,
                (native_next-ctypes.addressof(native_words))//4,
                bitstate.available_bits):
            raise AssertionError(
                f"Bitstream state mismatch case {case}: "
                f"original={(orig_cache,(orig_next-WORDS)//4,orig_available)} "
                f"native={(bitstate.cache,(native_next-ctypes.addressof(native_words))//4,bitstate.available_bits)}")
        checked += 1
    print(f"Bink entropy nibble-pair decoder original 0x{RVA:X} differential PASS: "
          f"{checked} valid synthetic Huffman streams")


if __name__ == "__main__":
    main()
