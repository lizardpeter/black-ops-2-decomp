#!/usr/bin/env python3
"""Differential original Bink FUN_180024FF0 vs reconstructed 16-bit delta/RLE.

The SHA256-pinned Store x64 binkw64.dll code runs in Unicorn using the
original 0x48-byte decoder and 0x18-byte LSB-first bitstream structures.
Our separately compiled native C++ code sees identical logical inputs.
We compare 16-bit output values, decoded byte-buffer states, bit cursor,
and the C++/original results against the independently encoded test values.

Synthetic well-formed streams cover signed/unsigned starting coefficients,
zero-width repeat groups, group sizes 1..8, signed deltas, zero deltas,
32-bit word crossings, and 16-bit result wrapping. This is NOT whole-frame
Bink playback, malformed-stream support or an ABI-complete Bink replacement.
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
from differential_entropy_bytes import Decoder, BitStream  # noqa: E402

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x24FF0
WORDS, OUTPUT = OBJECTS + 0x1000, OBJECTS + 0x3000
OUTPUT_BYTES = 160
WORDS_COUNT = 128


def build_case(rng, count, mode, width, group_widths):
    bit_values = []
    def append(value, bits):
        bit_values.extend((value >> i) & 1 for i in range(bits))

    append(count, 6)
    expected = []
    if count:
        initial_width = width - int(mode)
        initial = rng.randrange(1 << initial_width)
        if rng.randrange(7) == 0:
            initial = 0
        append(initial, initial_width)
        if mode and initial:
            sign = rng.randrange(2)
            append(sign, 1)
            if sign:
                initial = (-initial) & 0xFFFFFFFF
        running = initial
        expected.append(running & 0xFFFF)

        index = 1
        group = 0
        while index < count:
            group_count = min(count - index, 8)
            delta_width = group_widths[group % len(group_widths)]
            append(delta_width, 4)
            for _ in range(group_count):
                if delta_width:
                    mag = rng.randrange(1 << delta_width)
                    if rng.randrange(6) == 0:
                        mag = 0
                    append(mag, delta_width)
                    if mag:
                        negative = rng.randrange(2)
                        append(negative, 1)
                        running = (running - mag if negative
                                   else running + mag) & 0xFFFFFFFF
                expected.append(running & 0xFFFF)
            index += group_count
            group += 1

    words = [0] * WORDS_COUNT
    for bit_index, bit in enumerate(bit_values):
        words[bit_index // 32] |= bit << (bit_index % 32)
    return struct.pack("<" + "I" * WORDS_COUNT, *words), expected


def main():
    original = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "originals/dlls/binkw64.dll"
    reconstruction = Path(sys.argv[2]) if len(sys.argv) > 2 else ROOT / "build/bink_entropy_deltas16.so"
    if hashlib.sha256(original.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original Bink SHA-256 mismatch")
    image = Image(original)
    machine = Machine(image)
    native_library = ctypes.CDLL(str(reconstruction.resolve()))
    decoder = native_library.bink_decode_entropy_deltas16
    decoder.argtypes = (ctypes.POINTER(Decoder), ctypes.POINTER(BitStream),
                        ctypes.POINTER(ctypes.c_uint32))
    decoder.restype = None
    assert ctypes.sizeof(Decoder) == 0x48
    assert ctypes.sizeof(BitStream) == 0x18

    rng = random.Random(RVA)
    checked = 0
    cases = []
    for count in (0, 1, 2, 3, 8, 9, 10, 16, 17, 23, 31, 32, 47, 63):
        for mode in (0, 1):
            for baseline_width in (5, 9, 12, 16):
                for iteration in range(6):
                    widths = ([0] if iteration == 0 else
                              [1] if iteration == 1 else
                              [15] if iteration == 2 else
                              [0, 15] if iteration == 3 else
                              [3, 0, 6] if iteration == 4 else
                              [rng.randrange(16) for _ in range(8)])
                    cases.append((count, mode, baseline_width, widths))

    for case, (count, mode, start_width, widths) in enumerate(cases):
        words, expected = build_case(rng, count, mode, start_width, widths)
        expected_output = struct.pack(f"<{len(expected)}H", *expected)
        original_output_fill = bytes([0xA5]) * OUTPUT_BYTES
        original_decoder = bytearray(0x48)
        start_settings = (mode << 32) | start_width
        struct.pack_into("<QQQ", original_decoder, 0, OUTPUT, OUTPUT, start_settings)
        struct.pack_into("<IIQ", original_decoder, 0x38, 6, 126, OUTPUT)
        original_bits = bytearray(0x18)
        struct.pack_into("<Q", original_bits, 8, WORDS)

        def setup(writer):
            writer.write(WORDS, words)
            writer.write(OUTPUT, original_output_fill)

        value, buffers, writes, _ = machine.call(
            image.base + RVA,
            [bytes(original_decoder), bytes(original_bits), WORDS + len(words)],
            "void", setup=setup)
        if value is not None:
            raise AssertionError(f"Original 0x{RVA:X} fault case {case}: {value}")

        output_before = original_output_fill
        original_pixels = b"".join(
            writes.get(OUTPUT + i, output_before[i:i + 1])
            for i in range(OUTPUT_BYTES))
        if count and original_pixels[:2*count] != expected_output:
            mismatch = next((i for i, (a, b) in enumerate(
                zip(original_pixels[:2*count], expected_output)) if a != b), None)
            raise AssertionError(
                f"Original disagrees with encoded delta/RLE coefficients case {case} "
                f"count={count} mode={mode} start_width={start_width}: offset={mismatch}, "
                f"expected={expected_output.hex()}, actual={original_pixels[:2*count].hex()}")

        native_pixels = (ctypes.c_uint8 * OUTPUT_BYTES).from_buffer_copy(output_before)
        word_tuple = struct.unpack("<" + "I" * WORDS_COUNT, words)
        native_words = (ctypes.c_uint32 * WORDS_COUNT)(*word_tuple)
        ctx = Decoder()
        ctx.current = ctypes.cast(native_pixels, ctypes.POINTER(ctypes.c_uint8))
        ctx.end = ctypes.cast(native_pixels, ctypes.POINTER(ctypes.c_uint8))
        ctx.reserved10 = start_settings
        ctx.count_bits = 6
        ctx.capacity = 126
        ctx.buffer = ctypes.cast(native_pixels, ctypes.POINTER(ctypes.c_uint8))
        bitstream = BitStream()
        bitstream.next_word = ctypes.cast(native_words, ctypes.POINTER(ctypes.c_uint32))
        last_word = ctypes.cast(ctypes.byref(native_words, len(words)),
                                ctypes.POINTER(ctypes.c_uint32))
        decoder(ctypes.byref(ctx), ctypes.byref(bitstream), last_word)

        actual_pixels = ctypes.string_at(ctypes.addressof(native_pixels), OUTPUT_BYTES)
        if actual_pixels != original_pixels:
            byte_index = next(i for i, (a, b) in enumerate(zip(actual_pixels, original_pixels))
                              if a != b)
            raise AssertionError(
                f"Original/source 16-bit coefficient mismatch case={case}, "
                f"count={count}, mode={mode}, width={start_width}, group widths={widths}, "
                f"byte={byte_index}: original={original_pixels[byte_index]:#04x} "
                f"source={actual_pixels[byte_index]:#04x}")

        orig_current, orig_end = struct.unpack_from("<QQ", buffers[0], 0)
        src_start = ctypes.addressof(native_pixels)
        src_current = ctypes.cast(ctx.current, ctypes.c_void_p).value
        src_end = ctypes.cast(ctx.end, ctypes.c_void_p).value
        if (orig_current - OUTPUT, orig_end - OUTPUT) != (
                src_current - src_start, src_end - src_start):
            raise AssertionError(f"Output cursor mismatch case={case}")

        orig_cache, = struct.unpack_from("<I", buffers[1], 0)
        orig_word_ptr, = struct.unpack_from("<Q", buffers[1], 8)
        orig_available, = struct.unpack_from("<I", buffers[1], 16)
        native_word_ptr = ctypes.cast(bitstream.next_word, ctypes.c_void_p).value
        if (orig_cache, (orig_word_ptr - WORDS)//4, orig_available) != (
                bitstream.cache,
                (native_word_ptr - ctypes.addressof(native_words))//4,
                bitstream.available_bits):
            raise AssertionError(
                f"Bitstream cursor mismatch case={case}: "
                f"original={orig_cache,orig_word_ptr-WORDS,orig_available}, "
                f"source={bitstream.cache,native_word_ptr-ctypes.addressof(native_words),bitstream.available_bits}")
        checked += 1

    print(f"Bink 0x{RVA:X} 16-bit delta/RLE decoder differential PASS: "
          f"{checked} synthetic input cases")


if __name__ == "__main__":
    main()
