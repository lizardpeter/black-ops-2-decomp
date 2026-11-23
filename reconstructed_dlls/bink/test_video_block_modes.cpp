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
    // 16x16 mode 1/submode 6: a solid block with stride padding intact.
    constexpr int wide_stride=32;
    std::array<std::uint8_t,16*wide_stride> wide{};
    wide.fill(0xA5);
    bink_video_fill16x16(wide.data(),wide_stride,0x39);
    for(int y=0;y<16;++y) {
        for(int x=0;x<16;++x)assert(wide[y*wide_stride+x]==0x39);
        for(int x=16;x<wide_stride;++x)assert(wide[y*wide_stride+x]==0xA5);
    }
    // Original 16x16 submode 8 stores each 4 two-bit selectors in one
    // pattern byte and vertically doubles each 16-pixel source row.
    constexpr std::array<std::uint8_t,8> pattern16{
        0x00,0xff,0x55,0xaa,0x39,0x6c,0x12,0xd4
    };
    wide.fill(0xA5);
    bink_video_two_color16x16(wide.data(),wide_stride,0x10,0x39,
                               pattern16.data());
    for(int y=0;y<16;++y) {
        for(int x=0;x<16;++x) {
            const int code=(pattern16[y/2]>>(2*(x/4)))&3;
            const bool select_color1=((code>>(x%4/2))&1)!=0;
            assert(wide[y*wide_stride+x]==(select_color1?0x39:0x10));
        }
        for(int x=16;x<wide_stride;++x)assert(wide[y*wide_stride+x]==0xA5);
    }

}
