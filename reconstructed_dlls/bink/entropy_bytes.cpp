// Machine-code-derived reconstruction of Bink's Huffman byte block decoder.
// FUN_180024980, RVA 0x24980, pinned original Store binkw64.dll:
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// The original first reads a short bit-coded block length, then selects either
// a literal nibble repeated for the whole block or a nibble-indexed Huffman
// lookahead table with symbol map. This implementation preserves the actual
// 0x48-byte context and 24-byte bitreader layouts, the LSB-first 32-bit word
// refill, the 4-bit repeated-value branch, and the original 0x30/0x40 state
// offsets. No library decoder, compression library, or third-party source used.
//
// Precondition: caller supplies the original valid decoder configuration,
// aligned padded input words, <=31 count/lookahead bits, code lengths 1..15,
// enough input for the decoded output, and an output buffer >=capacity+4.
// Deliberately not a safe parser for untrusted arbitrary files by itself.
#include "entropy_bit_cursor.h"
#include <cstdint>

using t6_bink::internal::BitCursor;

extern "C" void bink_decode_entropy_bytes(t6_bink::ByteEntropyState* d,
                                           t6_bink::WordBitStream* bits,
                                           const std::uint32_t* word_end) {
    if (d->current != d->end) return;

    BitCursor cursor(*bits);
    const std::uint32_t count = cursor.read(d->count_bits);
    if (count == 0 || count > d->capacity) {
        d->current = d->buffer + 4;
        d->end = d->buffer;
        cursor.save();
        return;
    }
    d->current = d->buffer;
    d->end = d->buffer + count;

    const bool repeat = cursor.read(1) != 0;
    if (repeat) {
        const std::uint8_t literal = static_cast<std::uint8_t>(cursor.read(4));
        for (std::uint32_t i = 0; i < count; ++i) d->buffer[i] = literal;
        cursor.save();
        return;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        // The original takes this boundary check before a necessary lookahead
        // refill. Its higher-level video decoder is responsible for providing
        // safe padding for the other bit consumption paths.
        if (cursor.available() < d->lookup_bits && cursor.next() >= word_end) {
            d->current = d->buffer + 4;
            d->end = d->buffer;
            cursor.save();
            return;
        }
        const std::uint8_t packed = d->lookup_table[cursor.lookahead(d->lookup_bits)];
        const std::uint32_t code_length = packed >> 4;
        const std::uint8_t code_symbol = packed & 15U;
        cursor.consume(code_length);
        d->buffer[i] = d->symbol_map[code_symbol];
    }
    cursor.save();
}
