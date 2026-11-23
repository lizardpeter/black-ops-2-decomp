// Source-level regression for recovered inline Bink frame modes 6/8.
// Original real-frame comparisons: tests/dlls/differential_real_block_modes.py.
#include "video_block_modes.cpp"
#include <array>
#include <cassert>
#include <cstdint>

int main() {
    constexpr int stride=24;
    std::array<std::uint8_t,8*stride> out{};
    out.fill(0xA5);
    bink_video_fill8x8(out.data(),stride,0x39);
    for(int y=0;y<8;++y) {
        for(int x=0;x<8;++x) assert(out[y*stride+x]==0x39);
        for(int x=8;x<stride;++x) assert(out[y*stride+x]==0xA5);
    }
    constexpr std::array<std::uint8_t,8> pattern{
        0x00,0xff,0x01,0x80,0x55,0xaa,0x32,0xc6,
    };
    out.fill(0xA5);
    bink_video_two_color8x8(out.data(),stride,0x10,0x39,pattern.data());
    for(int y=0;y<8;++y) {
        for(int x=0;x<8;++x) {
            const std::uint8_t expected=((pattern[y]>>x)&1U)?0x39:0x10;
            assert(out[y*stride+x]==expected);
        }
        for(int x=8;x<stride;++x) assert(out[y*stride+x]==0xA5);
    }
    static_assert(original_color0_mask[0]==0xffffffffU);
    static_assert(original_color0_mask[15]==0x00000000U);
    static_assert(original_color1_mask[15]==0xffffffffU);
}
