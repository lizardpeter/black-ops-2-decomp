// Original Bink Store x64 DLL FUN_180024140 (RVA 0x24140):
// Huffman-decoded byte/literal/RLE stream reconstruction.
// SHA-256 of the pinned original DLL:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Original Ghidra pseudocode:
// * Read a count, reject zero or counts greater than the output capacity.
// * Read one mode bit: mode 1 fills the block with a 4-bit literal;
//   mode 0 decodes symbols from the Huffman lookahead and 16-byte symbol map.
// * Symbols 0..11 are literal byte values and update the previous byte.
// * Symbols 12..15 repeat the previous byte for 4, 8, 12, or 32 bytes.
//   Those four exact lengths were checked against original BINKCONS data
//   at RVAs 0x5A690..0x5A693 (the original accesses DAT_18005A684+symbol).
// * Preserve original 0x48-byte context, 24-byte bitreader, and word cursor.
//
// Only valid, initialized streams and RLE runs that fit the decoded block
// are currently reconstructed and compared; malformed stream handling is not
// represented as a complete decoder or safe file-parsing boundary.
// No FFmpeg, library codec or third-party decoder implementation is used.
#include "entropy_bit_cursor.h"
#include <cstdint>
#include <cstring>

extern "C" void bink_decode_entropy_byte_rle(
        t6_bink::ByteEntropyState* context,
        t6_bink::WordBitStream* stream,
        const std::uint32_t* word_end) {
    if (context->current != context->end) return;

    t6_bink::internal::BitCursor cursor(*stream);
    const std::uint32_t count = cursor.read(context->count_bits);
    if (count == 0 || count > context->capacity) {
        context->current = context->buffer + 4;
        context->end = context->buffer;
        cursor.save();
        return;
    }

    context->current = context->buffer;
    context->end = context->buffer + count;
    const bool repeated_literal = cursor.read(1) != 0;
    if (repeated_literal) {
        const auto literal = static_cast<std::uint8_t>(cursor.read(4));
        std::memset(context->buffer, literal, count);
        cursor.save();
        return;
    }

    constexpr std::uint8_t original_run_lengths[4] = {4, 8, 12, 32};
    std::uint8_t previous = 0;
    std::uint32_t position = 0;
    while (position < count) {
        if (cursor.available() < context->lookup_bits &&
            cursor.next() >= word_end) {
            context->current = context->buffer + 4;
            context->end = context->buffer;
            cursor.save();
            return;
        }
        const std::uint8_t packed =
            context->lookup_table[cursor.lookahead(context->lookup_bits)];
        cursor.consume(packed >> 4);
        const std::uint8_t symbol = context->symbol_map[packed & 15U];

        if (symbol < 12U) {
            previous = symbol;
            context->buffer[position++] = previous;
        } else {
            const std::uint32_t run = original_run_lengths[symbol - 12U];
            if (run > count - position)
                break; // Original stops decoding an overlong run.
            std::memset(context->buffer + position, previous, run);
            position += run;
        }
    }
    cursor.save();
}
