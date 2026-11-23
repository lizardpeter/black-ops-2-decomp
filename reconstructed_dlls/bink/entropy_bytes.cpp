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
#include "entropy_bytes.h"
#include <cstdint>

namespace {
using t6_bink::WordBitStream;

// Original 32-bit LSB-first bit reservoir: a refill ORs one little-endian
// word at bit offset available_bits then advances next_word by four bytes.
// This does not enforce an artificial byte-alignment boundary.
void refill(WordBitStream& s) {
    const auto next = *s.next_word++;
    s.cache = static_cast<std::uint32_t>(
        std::uint64_t(s.cache) | (std::uint64_t(next) << s.available_bits));
    s.available_bits += 32;
}
std::uint32_t peek(WordBitStream& s, std::uint32_t width) {
    if (s.available_bits < width) refill(s);
    return s.cache & ((std::uint32_t{1} << width) - 1U);
}
void consume(WordBitStream& s, std::uint32_t width) {
    s.cache >>= width;
    s.available_bits -= width;
}
std::uint32_t read(WordBitStream& s, std::uint32_t width) {
    const auto result = peek(s, width);
    consume(s, width);
    return result;
}
}

extern "C" void bink_decode_entropy_bytes(t6_bink::ByteEntropyState* d,
                                           t6_bink::WordBitStream* bits,
                                           const std::uint32_t* word_end) {
    if (d->current != d->end) return;

    const std::uint32_t count = read(*bits, d->count_bits);
    if (count == 0 || count > d->capacity) {
        d->current = d->buffer + 4;
        d->end = d->buffer;
        return;
    }
    d->current = d->buffer;
    d->end = d->buffer + count;

    const bool repeat = read(*bits, 1) != 0;
    if (repeat) {
        const std::uint8_t literal = static_cast<std::uint8_t>(read(*bits, 4));
        for (std::uint32_t i = 0; i < count; ++i) d->buffer[i] = literal;
        return;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        // The original takes this boundary check before a necessary lookahead
        // refill. Its higher-level video decoder is responsible for providing
        // safe padding for the other bit consumption paths.
        if (bits->available_bits < d->lookup_bits && bits->next_word >= word_end) {
            d->current = d->buffer + 4;
            d->end = d->buffer;
            return;
        }
        const std::uint8_t packed = d->lookup_table[peek(*bits, d->lookup_bits)];
        const std::uint32_t code_length = packed >> 4;
        const std::uint8_t code_symbol = packed & 15U;
        if (bits->available_bits < code_length) refill(*bits);
        consume(*bits, code_length);
        d->buffer[i] = d->symbol_map[code_symbol];
    }
}
