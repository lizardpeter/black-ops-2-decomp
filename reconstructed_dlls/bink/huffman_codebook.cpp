// Source-level reconstruction of original Bink codebook initializer
// FUN_180023840, RVA 0x23840, from pinned Microsoft Store binkw64.dll:
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Inputs: the original 24-byte LSB-first WordBitStream, a pointer-output
// for the selected 16-type Huffman lookup table, a 32-bit lookup width
// output, and the 16-byte symbol permutation output.
//
// Original code starts with a 4-bit codebook type, selecting ORIGINAL
// immutable BINKCONS bytes at 0x5AD40 plus original offsets at 0x5A800.
// Type 0 uses identity symbol mapping. Other types encode a symbol
// permutation as either: (1) an explicit unique nibble prefix followed
// by remaining ascending symbols; or (2) a hierarchy of stable merges
// of 1, 2, 4 and 8 element runs driven by bit masks, using the exact
// selection semantics of original FUN_1800237B0.
//
// Unlike the Ghidra compiler output, this version uses a generic merge
// loop rather than reproducing its unrolled/de-duplicated jump layout.
// No FFmpeg or third-party Bink source is used. Invalid repeated symbols
// in explicit-prefix mode were not supported by the original codec.
//
// Requires the same padding/valid-initialization guarantees as the
// original internal routine. This is not an external Bink ABI or player.
#include "entropy_bit_cursor.h"
#include "original_huffman_tables.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace t6_bink::internal {
// Original FUN_1800237B0 consumes a selection bit for each output until
// either of the two equal-sized inputs has been exhausted, then copies
// the rest without additional bits. A zero chooses the left half.
static std::uint32_t stable_merge(const std::uint8_t* left,
                                  const std::uint8_t* right,
                                  std::uint32_t half,
                                  std::uint32_t mask,
                                  std::uint8_t* out) {
    std::uint32_t l=0, r=0, used=0;
    while (l<half && r<half) {
        // Compute the destination index BEFORE incrementing a source index.
        // Assignment evaluates its right-hand expression first in C++17+;
        // "out[l+r] = right[r++]" would otherwise skip the first output.
        const auto dest=l+r;
        if (mask & 1U) out[dest]=right[r++];
        else out[dest]=left[l++];
        mask >>= 1;
        ++used;
    }
    while (l<half) out[l+r] = left[l], ++l;
    while (r<half) out[l+r] = right[r], ++r;
    return used;
}
} // namespace t6_bink::internal

extern "C" const std::uint8_t* bink_original_huffman_lookup_base() {
    return t6_bink::original_tables::packed_lookups.data();
}

extern "C" void bink_init_huffman_codebook(
        t6_bink::WordBitStream* stream,
        const std::uint8_t** lookup,
        std::uint32_t* lookup_bits,
        std::uint8_t* symbol_map) {
    t6_bink::internal::BitCursor bits(*stream);
    const std::uint32_t type = bits.read(4);
    *lookup = t6_bink::original_tables::packed_lookups.data() +
              t6_bink::original_tables::offsets[type];
    *lookup_bits = t6_bink::original_tables::widths[type];

    if (type == 0) {
        for (std::uint32_t i=0;i<16;++i)
            symbol_map[i] = static_cast<std::uint8_t>(i);
        bits.save();
        return;
    }

    if (bits.read(1) != 0) {
        const std::uint32_t explicit_count = bits.read(3) + 1;
        std::uint32_t unused = 0xffffU;
        for (std::uint32_t i=0;i<explicit_count;++i) {
            const auto value = static_cast<std::uint8_t>(bits.read(4));
            symbol_map[i] = value;
            unused &= ~(1U << value);
        }
        std::uint32_t write = explicit_count;
        for (std::uint32_t value=0;value<16;++value)
            if ((unused >> value) & 1U)
                symbol_map[write++] = static_cast<std::uint8_t>(value);
        bits.save();
        return;
    }

    // Two-bit depth chooses how many permutation stages to apply after
    // independently swapping each adjacent pair. 0=pair swaps only,
    // 1=merge pairs to 4, 2=also merge into 8, 3=also merge into 16.
    const std::uint32_t depth = bits.read(2);
    for (std::uint32_t pair=0;pair<8;++pair) {
        const auto a = static_cast<std::uint8_t>(2*pair);
        const auto swap = bits.read(1);
        symbol_map[2*pair] = static_cast<std::uint8_t>(a + swap);
        symbol_map[2*pair+1] = static_cast<std::uint8_t>(a + (swap ^ 1U));
    }

    std::array<std::uint8_t,16> temp{};
    for (std::uint32_t stage=1;stage<=depth;++stage) {
        const std::uint32_t half = 1U << stage;
        const std::uint32_t total = 2U * half;
        for (std::uint32_t i=0;i<16;i+=total) {
            const std::uint32_t mask = bits.lookahead(total-1);
            const std::uint32_t consumed = t6_bink::internal::stable_merge(
                    symbol_map+i, symbol_map+i+half, half, mask,
                    temp.data()+i);
            bits.consume(consumed);
        }
        std::memcpy(symbol_map, temp.data(), 16);
    }
    bits.save();
}
