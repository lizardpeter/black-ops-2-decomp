// Source-level regression cases for original Bink nibble-pair Huffman stage.
// Exact pinned-x64 comparison is tests/dlls/differential_entropy_nibbles.py.
#include "entropy_nibbles.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

static void push_bits(std::array<std::uint32_t,16>& words, std::size_t& at,
                      std::uint32_t value, unsigned width) {
    for (unsigned bit = 0; bit < width; ++bit, ++at)
        if (value & (1U << bit)) words[at/32] |= (1U << (at%32));
}
int main() {
    std::array<std::uint32_t,16> words{};
    std::size_t at = 0;
    push_bits(words, at, 4, 6); // Four output bytes
    for (std::uint32_t code = 1; code <= 8; ++code)
        push_bits(words, at, code, 4);
    std::uint8_t lookup[16], storage[64];
    for (unsigned i = 0; i < 16; ++i) lookup[i] = std::uint8_t(0x40U | i);
    for (auto& b : storage) b = 0xA5;
    t6_bink::ByteEntropyState state{
        storage, storage, 0, {}, 4, 0, lookup, 6, 63, storage
    };
    for (unsigned i = 0; i < 16; ++i) state.symbol_map[i] = std::uint8_t(i);
    t6_bink::WordBitStream bits{0, 0, words.data(), 0, 0};
    bink_decode_entropy_nibbles(&state, &bits, words.data() + words.size());
    assert(state.current == storage && state.end == storage + 4);
    assert(storage[0] == 0x21 && storage[1] == 0x43);
    assert(storage[2] == 0x65 && storage[3] == 0x87);
    for (unsigned i = 4; i < 64; ++i) assert(storage[i] == 0xA5);

    // An unconsumed decoded block must be returned without advancing bits.
    const auto old_cache = bits.cache;
    const auto old_ptr = bits.next_word;
    const auto old_avail = bits.available_bits;
    state.current = storage;
    state.end = storage + 4;
    bink_decode_entropy_nibbles(&state, &bits, words.data() + words.size());
    assert(bits.cache == old_cache && bits.next_word == old_ptr &&
           bits.available_bits == old_avail);
}
