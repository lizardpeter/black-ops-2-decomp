// Source-only safety and zero-block tests for original Bink 8x8 transform.
// Exact original-versus-reconstruction testing is in
// tests/dlls/differential_video_inverse8x8.py.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include "video_inverse8x8.cpp"

int main() {
    std::array<std::int16_t, 64> coeff{};
    std::array<std::int32_t, 64> quant{};
    quant.fill(2048);
    for (int stride : {8, 16, 32}) {
        std::array<std::uint8_t, 256> out;
        out.fill(0xA5);
        bink_video_inverse8x8(out.data(), stride, coeff.data(), quant.data());
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col)
                assert(out[row * stride + col] == 0);
            for (int col = 8; col < stride; ++col)
                assert(out[row * stride + col] == 0xA5);
        }
    }
    coeff[0] = 1024;
    std::array<std::uint8_t, 128> first{}, second{};
    bink_video_inverse8x8(first.data(), 16, coeff.data(), quant.data());
    bink_video_inverse8x8(second.data(), 16, coeff.data(), quant.data());
    assert(first == second);
    assert(std::any_of(first.begin(), first.end(),
                       [](std::uint8_t v) {return v != 0;}));
}
