// Reconstructed original Bink video entropy byte-pair decoder,
// FUN_180024B80 at RVA 0x24B80, from Store binkw64.dll x64 machine code.
// SHA256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Each decoded byte is formed from TWO independently Huffman-decoded nibble
// symbols: low_symbol | (high_symbol << 4). The function uses the same
// original Huffman lookup table, symbol map, block-count framing, and 32-bit
// LSB-first bitstream fields as FUN_180024980.
//
// No external decoder or codec source is used. The original assumes a valid
// initialized Huffman table, count width 1..31, sufficient padded input words
// and destination capacity. This is not a standalone parser for untrusted files.
#include "entropy_bit_cursor.h"
#include <cstdint>

extern "C" void bink_decode_entropy_nibbles(
        t6_bink::ByteEntropyState* context,
        t6_bink::WordBitStream* stream,
        const std::uint32_t* word_end) {
    if (context->current != context->end) return;

    t6_bink::internal::BitCursor cursor(*stream);
    const std::uint32_t count = cursor.read(context->count_bits);
    if (!count || count > context->capacity) {
        context->current = context->buffer + 4;
        context->end = context->buffer;
        cursor.save();
        return;
    }

    context->current = context->buffer;
    context->end = context->buffer + count;

    for (std::uint32_t i = 0; i < count; ++i) {
        // The original only checks the input-end pointer on the FIRST symbol
        // when a table lookahead would need a new word.
        if (cursor.available() < context->lookup_bits &&
            cursor.next() >= word_end) {
            context->current = context->buffer + 4;
            context->end = context->buffer;
            cursor.save();
            return;
        }
        const auto packed_low =
            context->lookup_table[cursor.lookahead(context->lookup_bits)];
        cursor.consume(packed_low >> 4);
        const std::uint8_t low = context->symbol_map[packed_low & 15U];

        const auto packed_high =
            context->lookup_table[cursor.lookahead(context->lookup_bits)];
        cursor.consume(packed_high >> 4);
        const std::uint8_t high = context->symbol_map[packed_high & 15U];

        context->buffer[i] = static_cast<std::uint8_t>(
            static_cast<std::uint32_t>(low) |
            (static_cast<std::uint32_t>(high) << 4));
    }
    cursor.save();
}
