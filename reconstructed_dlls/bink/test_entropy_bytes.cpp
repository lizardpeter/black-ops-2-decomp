// Standalone original-layout tests for FUN_180024980's byte entropy decoding.
// Original-machine differential: tests/dlls/differential_entropy_bytes.py.
#include "entropy_bytes.cpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

static void append(std::array<std::uint32_t,16>& words, std::size_t& cursor,
                   std::uint32_t value, unsigned bits) {
    for (unsigned i = 0; i < bits; ++i, ++cursor)
        if (value & (1u << i)) words[cursor / 32] |= 1u << (cursor % 32);
}

int main() {
    std::uint8_t table[16];
    std::uint8_t symbols[16];
    for (unsigned i = 0; i < 16; ++i) {
        table[i] = static_cast<std::uint8_t>(0x40 | i);
        symbols[i] = static_cast<std::uint8_t>(100 + i);
    }
    for (unsigned count : {1u, 7u, 25u, 48u}) {
        for (bool repeat : {false, true}) {
            std::array<std::uint32_t,16> words{};
            std::size_t cursor = 0;
            append(words, cursor, count, 6);
            append(words, cursor, unsigned(repeat), 1);
            for (unsigned i = 0; i < (repeat ? 1u : count); ++i)
                append(words, cursor, (i * 7u + 3u) & 15u, 4);
            std::array<std::uint8_t, 64> storage{};
            storage.fill(0xA5);
            t6_bink::ByteEntropyState decoder{
                storage.data(), storage.data(), 0, {}, 4, 0, table, 6, 63, storage.data()
            };
            for (unsigned i = 0; i < 16; ++i) decoder.symbol_map[i] = symbols[i];
            t6_bink::WordBitStream stream{0, 0, words.data(), 0, 0};
            bink_decode_entropy_bytes(&decoder, &stream, words.data() + words.size());
            assert(decoder.current == storage.data());
            assert(decoder.end == storage.data() + count);
            for (unsigned i = 0; i < count; ++i) {
                auto want = repeat ? std::uint8_t(3) :
                           symbols[(i * 7u + 3u) & 15u];
                assert(storage[i] == want);
            }
            for (unsigned i = count; i < storage.size(); ++i) assert(storage[i] == 0xA5);
            // Cached words and available bits are consistent with exact stream use.
            assert(stream.available_bits == (32u - (cursor % 32u)) % 32u);
        }
    }
    std::array<std::uint32_t,16> words{};
    std::array<std::uint8_t, 64> storage{};
    t6_bink::ByteEntropyState invalid{storage.data(), storage.data(), 0, {},
                                     4, 0, table, 6, 63, storage.data()};
    t6_bink::WordBitStream stream{0, 0, words.data(), 0, 0};
    bink_decode_entropy_bytes(&invalid, &stream, words.data() + words.size());
    assert(invalid.current == storage.data() + 4 && invalid.end == storage.data());
}
