#pragma once
#include <cstddef>
#include <cstdint>

namespace t6_bink {
// Derived from original FUN_180024980's field accesses. Do not reorder fields.
struct ByteEntropyState {
    std::uint8_t* current;               // +0x00, original decoded read head
    std::uint8_t* end;                   // +0x08
    std::uint64_t reserved10;            // +0x10
    std::uint8_t symbol_map[16];         // +0x18, Huffman nibble -> byte
    std::uint32_t lookup_bits;           // +0x28, Huffman lookahead width
    std::uint32_t reserved2c;            // +0x2C
    const std::uint8_t* lookup_table;    // +0x30, packed length/index
    std::uint32_t count_bits;            // +0x38, count code width
    std::uint32_t capacity;              // +0x3C, max decoded byte count
    std::uint8_t* buffer;                // +0x40, destination
};
static_assert(sizeof(ByteEntropyState) == 0x48);
static_assert(offsetof(ByteEntropyState, symbol_map) == 0x18);
static_assert(offsetof(ByteEntropyState, lookup_bits) == 0x28);
static_assert(offsetof(ByteEntropyState, lookup_table) == 0x30);
static_assert(offsetof(ByteEntropyState, count_bits) == 0x38);
static_assert(offsetof(ByteEntropyState, capacity) == 0x3C);
static_assert(offsetof(ByteEntropyState, buffer) == 0x40);

// Original bitreader record from FUN_180024980 (little-endian 32-bit cache).
struct WordBitStream {
    std::uint32_t cache;                 // +0
    std::uint32_t reserved4;             // +4
    const std::uint32_t* next_word;      // +8
    std::uint32_t available_bits;        // +16
    std::uint32_t reserved14;            // +20
};
static_assert(sizeof(WordBitStream) == 24);
static_assert(offsetof(WordBitStream, next_word) == 8);
static_assert(offsetof(WordBitStream, available_bits) == 16);
}

// Original FUN_180024980, internal Bink entropy byte-block decoder.
// Operates on the original state layouts, not a public Bink export.
// Only call on valid initialized structures, widths (1..31), non-overlapping
// buffers and a bitstream padded as the original caller requires.
extern "C" void bink_decode_entropy_bytes(t6_bink::ByteEntropyState* context,
                                           t6_bink::WordBitStream* bits,
                                           const std::uint32_t* word_end);
