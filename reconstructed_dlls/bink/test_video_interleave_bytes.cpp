// Standalone tests of the original Bink byte stream interleaver.
// Full original x64 differential test: tests/dlls/differential_video_interleave.py.
#include <array>
#include <cassert>
#include <cstdint>
#include "video_interleave_bytes.cpp"

int main() {
    const std::array<std::uint8_t, 4> first = {10, 11, 12, 13};
    const std::array<std::uint8_t, 4> second = {20, 21, 22, 23};
    std::array<std::uint8_t, 8> out{};
    const int consumed = bink_video_interleave_bytes(0b0101, out.data(),
                                                     first.data(), second.data(), 4);
    assert(consumed == 6);
    assert((out == std::array<std::uint8_t,8>{20,10,21,11,12,13,22,23}));

    out.fill(0);
    assert(bink_video_interleave_bytes(0, out.data(), first.data(), second.data(), 4) == 4);
    assert((out == std::array<std::uint8_t,8>{10,11,12,13,20,21,22,23}));
    out.fill(0);
    assert(bink_video_interleave_bytes(~std::uint64_t{0}, out.data(),
                                       first.data(), second.data(), 4) == 4);
    assert((out == std::array<std::uint8_t,8>{20,21,22,23,10,11,12,13}));
    std::array<std::uint8_t, 2> single{};
    const std::array<std::uint8_t, 1> a = {7}, b = {8};
    assert(bink_video_interleave_bytes(1, single.data(), a.data(), b.data(), 1) == 1);
    assert(single[0] == 8 && single[1] == 7);
}
