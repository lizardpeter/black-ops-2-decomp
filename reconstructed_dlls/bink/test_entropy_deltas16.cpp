// Standalone C++ tests of original 0x24FF0 16-bit delta/RLE reconstruction.
// Native-vs-pinned-original differential runs in tests/dlls/.
#include "entropy_deltas16.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
struct Packer {
    std::array<std::uint32_t, 64> words{};
    std::size_t position = 0;
    void push(std::uint32_t value, unsigned count) {
        for (unsigned bit = 0; bit < count; ++bit, ++position)
            if ((value >> bit) & 1U)
                words[position / 32] |= 1U << (position % 32);
    }
};
std::uint16_t at(const std::array<std::uint8_t, 128>& bytes, unsigned index) {
    std::uint16_t value;
    std::memcpy(&value, bytes.data() + 2U * index, sizeof(value));
    return value;
}
void run(Packer& p, std::uint32_t count, std::uint32_t start_width,
         std::uint32_t mode, const std::vector<std::uint16_t>& expected) {
    std::array<std::uint8_t, 128> decoded{};
    decoded.fill(0xA5);
    t6_bink::ByteEntropyState ctx{
        decoded.data(), decoded.data(),
        (std::uint64_t(mode) << 32) | start_width,
        {}, 0, 0, nullptr, 6, 126, decoded.data()
    };
    t6_bink::WordBitStream input{0, 0, p.words.data(), 0, 0};
    bink_decode_entropy_deltas16(&ctx, &input, p.words.data() + p.words.size());
    assert(ctx.current == decoded.data());
    assert(ctx.end == decoded.data() + count * 2U);
    assert(expected.size() == count);
    for (unsigned i = 0; i < count; ++i) assert(at(decoded, i) == expected[i]);
    for (std::size_t i = count * 2U; i < decoded.size(); ++i)
        assert(decoded[i] == 0xA5);
}
}

int main() {
    // A zero-width group repeats the starting coefficient 8 times, then
    // a second group applies signed 5-bit deltas to the final 8 coefficients.
    {
        Packer p;
        p.push(17, 6);
        p.push(1023, 12);
        p.push(0, 4);
        p.push(5, 4);
        std::vector<std::uint16_t> out(9, 1023);
        std::uint32_t running = 1023;
        const std::array<int, 8> deltas = {3, -2, 0, 4, -1, 7, -5, 1};
        for (int delta : deltas) {
            const unsigned mag = unsigned(delta < 0 ? -delta : delta);
            p.push(mag, 5);
            if (mag != 0) p.push(delta < 0, 1);
            running += static_cast<std::uint32_t>(delta);
            out.push_back(static_cast<std::uint16_t>(running));
        }
        run(p, 17, 12, 0, out);
    }
    // Signed baseline: the sign bit only appears for nonzero magnitude.
    // The baseline and all later deltas wrap into 16-bit outputs.
    {
        Packer p;
        p.push(10, 6);
        p.push(19, 8); p.push(1, 1);  // Signed baseline -19, width 9
        p.push(4, 4);                 // Group of 8
        std::uint32_t last = std::uint32_t{0} - 19U;
        std::vector<std::uint16_t> out = {static_cast<std::uint16_t>(last)};
        for (int delta : {1, -7, 0, -15, 12, -3, 1, 0}) {
            const unsigned mag = unsigned(delta < 0 ? -delta : delta);
            p.push(mag, 4);
            if (mag) p.push(delta < 0, 1);
            last += static_cast<std::uint32_t>(delta);
            out.push_back(static_cast<std::uint16_t>(last));
        }
        p.push(0, 4); // Final, one-element repeat group
        out.push_back(static_cast<std::uint16_t>(last));
        run(p, 10, 9, 1, out);
    }
    // Single coefficient: no group width or delta bits consumed.
    {
        Packer p;
        p.push(1, 6);
        p.push(0x7FFF, 15);
        run(p, 1, 15, 0, {0x7FFF});
    }
    // Count zero must return original error sentinel pointers.
    {
        Packer p;
        p.push(0, 6);
        std::array<std::uint8_t, 128> dest{};
        dest.fill(0xA5);
        t6_bink::ByteEntropyState ctx{
            dest.data(), dest.data(), 12, {}, 0, 0, nullptr, 6, 126, dest.data()
        };
        t6_bink::WordBitStream bits{0, 0, p.words.data(), 0, 0};
        bink_decode_entropy_deltas16(&ctx, &bits, p.words.data() + p.words.size());
        assert(ctx.current == dest.data() + 4);
        assert(ctx.end == dest.data());
    }
}
