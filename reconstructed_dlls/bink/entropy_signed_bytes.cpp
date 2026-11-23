// Machine-code-derived reconstruction of Bink's signed-byte entropy decoder.
// FUN_180024DB0 at RVA 0x24DB0, original Store binkw64.dll SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Decodes a bit-coded output count, a one-bit mode, then either:
//  - Huffman symbols from the original lookahead table; for each nonzero
//    symbol, consume an additional sign bit and negate modulo 256, or
//  - one repeated 4-bit magnitude, followed by a sign bit when nonzero.
// All bits are consumed LSB first from the same 32-bit word/64-bit temporary
// reservoir as the two other reconstructed Bink video entropy decoders.
//
// Original context and bitreader field offsets are reused unchanged.
// No third-party Bink/FFmpeg/codec implementation is used.
//
// Preconditions: original decoder state, valid lookup-table widths 1..31,
// valid 1..15-bit Huffman lengths, padded input words and sufficient output.
// This is a faithful internal routine for valid inputs, not an untrusted-file
// boundary or a complete Bink player.
#include "entropy_bit_cursor.h"
#include <cstdint>

extern "C" void bink_decode_entropy_signed_bytes(
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

    const bool repeated = cursor.read(1) != 0;
    if (repeated) {
        std::uint8_t value = static_cast<std::uint8_t>(cursor.read(4));
        if (value && cursor.read(1))
            value = static_cast<std::uint8_t>(0U - value);
        for (std::uint32_t i = 0; i < count; ++i)
            context->buffer[i] = value;
        cursor.save();
        return;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        if (cursor.available() < context->lookup_bits &&
            cursor.next() >= word_end) {
            context->current = context->buffer + 4;
            context->end = context->buffer;
            cursor.save();
            return;
        }
        const auto packed =
            context->lookup_table[cursor.lookahead(context->lookup_bits)];
        cursor.consume(packed >> 4);
        std::uint8_t value = context->symbol_map[packed & 15U];
        if (value && cursor.read(1))
            value = static_cast<std::uint8_t>(0U - value);
        context->buffer[i] = value;
    }
    cursor.save();
}
