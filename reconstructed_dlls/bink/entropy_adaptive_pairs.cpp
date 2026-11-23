// Actual Bink video-plane adaptive Huffman pair decoders, Store Bink x64:
//   FUN_1800246A0: output raw packed nibble byte
//   FUN_1800243B0: output signed 7-bit magnitude biased by +0x80
// Original SHA256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Both routines are reached 32 times each (depending on flags) during
// ORIGINAL decoding of the first real la_pip_seq_6.bik luma frame.
// This is a source-level transcription of the Ghidra machine instructions,
// NOT Bink SDK/FFmpeg/library decoder source.
//
// The adaptive record comes directly from FUN_1800252C0's stack:
// +0x000 16*16 byte symbol maps, +0x100 16 u32 Huffman widths,
// +0x140 16 64-bit lookup-table pointers, +0x1C0 current symbol/index.
// The "high" nibble selects the NEXT adaptive Huffman table. The "low"
// nibble comes from the ordinary 0x48 ByteEntropyState Huffman table.
// An encoded mode bit selects between decoding each output symbol and
// repeating one decoded byte for the entire count.
//
// Preconditions: valid original codebook records, buffer space and padded
// input; the high-codeword length/width and low-codeword length/width
// obey the original Huffman table contracts. This is not a safe standalone
// parser for malicious media and is not a complete Bink frame decoder.
#include "entropy_bit_cursor.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace t6_bink {
struct AdaptiveNibbleBooks {
    std::uint8_t symbol_map[16][16];             // +0x000
    std::uint32_t lookup_bits[16];               // +0x100
    const std::uint8_t* lookup_table[16];        // +0x140
    std::uint32_t current_symbol;                // +0x1C0
};
static_assert(offsetof(AdaptiveNibbleBooks,lookup_bits)==0x100);
static_assert(offsetof(AdaptiveNibbleBooks,lookup_table)==0x140);
static_assert(offsetof(AdaptiveNibbleBooks,current_symbol)==0x1C0);
}

namespace {
void decode_adaptive_pair(t6_bink::ByteEntropyState* context,
                          t6_bink::WordBitStream* stream,
                          t6_bink::AdaptiveNibbleBooks* books,
                          const std::uint32_t* word_end,
                          bool signed_magnitude) {
    if (context->current != context->end) return;

    t6_bink::internal::BitCursor bits(*stream);
    const std::uint32_t count=bits.read(context->count_bits);
    if (count==0 || count>context->capacity) {
        context->current=context->buffer+4;
        context->end=context->buffer;
        bits.save();
        return;
    }
    context->current=context->buffer;
    context->end=context->buffer+count;
    const bool repeat=bits.read(1)!=0;
    const std::uint32_t symbols_to_decode=repeat?1U:count;

    for (std::uint32_t i=0;i<symbols_to_decode;++i) {
        const std::uint32_t book=books->current_symbol;
        const std::uint32_t high_bits=books->lookup_bits[book];
        if (bits.available()<high_bits && bits.next()>=word_end) {
            context->current=context->buffer+4;
            context->end=context->buffer;
            bits.save();
            return;
        }
        const std::uint8_t packed_high =
            books->lookup_table[book][bits.lookahead(high_bits)];
        bits.consume(packed_high>>4);
        const std::uint8_t high=books->symbol_map[book][packed_high&0x0FU];
        books->current_symbol=high;

        const std::uint8_t packed_low =
            context->lookup_table[bits.lookahead(context->lookup_bits)];
        bits.consume(packed_low>>4);
        const std::uint8_t low=context->symbol_map[packed_low&0x0FU];

        std::uint8_t result=static_cast<std::uint8_t>(
            (std::uint32_t(high)<<4)|std::uint32_t(low));
        if (signed_magnitude) {
            const std::uint8_t magnitude=result&0x7FU;
            const std::int32_t signed_value=(result&0x80U)?
                -std::int32_t(magnitude):std::int32_t(magnitude);
            result=static_cast<std::uint8_t>(signed_value+0x80);
        }
        context->buffer[i]=result;
    }
    if (repeat && count>1)
        std::memset(context->buffer,context->buffer[0],count);
    bits.save();
}
}

extern "C" void bink_decode_adaptive_nibbles(
        t6_bink::ByteEntropyState* context,
        t6_bink::WordBitStream* stream,
        t6_bink::AdaptiveNibbleBooks* books,
        const std::uint32_t* word_end) {
    decode_adaptive_pair(context,stream,books,word_end,false);
}

extern "C" void bink_decode_adaptive_nibbles_signed(
        t6_bink::ByteEntropyState* context,
        t6_bink::WordBitStream* stream,
        t6_bink::AdaptiveNibbleBooks* books,
        const std::uint32_t* word_end) {
    decode_adaptive_pair(context,stream,books,word_end,true);
}
