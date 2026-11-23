// Source-only tests for decompiled Bink FUN_18002F630 (2x inverse block).
// Exact original-binary oracle is tests/dlls/differential_video_inverse8x8_2x.py.
#include <array>
#include <cassert>
#include <cstdint>
#include "video_inverse8x8.cpp"
#include "video_inverse8x8_2x.cpp"

int main() {
    std::array<std::int16_t, 64> coeff{};
    std::array<std::int32_t, 64> quant{};
    quant.fill(2048);
    for (int trial = 0; trial < 8; ++trial) {
        for (int i = 0; i < 64; ++i)
            coeff[i] = static_cast<std::int16_t>((i + trial * 11) % 31 - 15);
        std::array<std::uint8_t, 64> plain{};
        bink_video_inverse8x8(plain.data(), 8, coeff.data(), quant.data());
        for (int stride : {16, 24, 32}) {
            std::array<std::uint8_t, 512> upscaled;
            upscaled.fill(0xA5);
            bink_video_inverse8x8_2x(upscaled.data(), stride, coeff.data(), quant.data());
            for (int y = 0; y < 16; ++y) {
                for (int x = 0; x < 16; ++x)
                    assert(upscaled[y * stride + x] == plain[(y / 2) * 8 + (x / 2)]);
                for (int x = 16; x < stride; ++x)
                    assert(upscaled[y * stride + x] == 0xA5);
            }
        }
    }
}
