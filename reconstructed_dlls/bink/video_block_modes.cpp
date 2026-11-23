// Reconstructed inline 8x8 modes from original Bink frame function
// FUN_1800252C0: mode 6 (one-color fill), mode 8 (two-color mask).
// The original 16-bit color stream supplies only the low byte for pixel
// values; mode 8 consumes 8 bytes of two 4-bit pixel masks, one per row.
// All 16 mask dwords were recovered byte-for-byte from the pinned original
// Store binkw64.dll, BINKCONS RVAs 0x5A6C0 and 0x5A700.
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// This is source recovered from ORIGINAL machine code, not FFmpeg or another
// preexisting video decoder. Input pointers/stride follow valid original
// 8x8 frame block invariants, not arbitrary untrusted-media parsing.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {
constexpr std::array<std::uint32_t,16> original_color0_mask = {
    0xffffffffU,0xffffff00U,0xffff00ffU,0xffff0000U,
    0xff00ffffU,0xff00ff00U,0xff0000ffU,0xff000000U,
    0x00ffffffU,0x00ffff00U,0x00ff00ffU,0x00ff0000U,
    0x0000ffffU,0x0000ff00U,0x000000ffU,0x00000000U,
};
constexpr std::array<std::uint32_t,16> original_color1_mask = {
    0x00000000U,0x000000ffU,0x0000ff00U,0x0000ffffU,
    0x00ff0000U,0x00ff00ffU,0x00ffff00U,0x00ffffffU,
    0xff000000U,0xff0000ffU,0xff00ff00U,0xff00ffffU,
    0xffff0000U,0xffff00ffU,0xffffff00U,0xffffffffU,
};
static_assert((original_color0_mask[11]|original_color1_mask[11])==0xffffffffU);
std::uint32_t repeated(std::uint8_t v) {
    return std::uint32_t(v)*0x01010101U;
}
}

extern "C" void bink_video_fill8x8(std::uint8_t* out,
                                      std::int32_t stride,
                                      std::uint8_t color) {
    for (int y=0;y<8;++y) {
        std::memset(out+std::ptrdiff_t(y)*stride,color,8);
    }
}

extern "C" void bink_video_two_color8x8(std::uint8_t* out,
                                           std::int32_t stride,
                                           std::uint8_t color0,
                                           std::uint8_t color1,
                                           const std::uint8_t* pattern) {
    const std::uint32_t c0=repeated(color0);
    const std::uint32_t c1=repeated(color1);
    for (int y=0;y<8;++y) {
        const std::uint8_t packed=pattern[y];
        const std::uint32_t low=packed&15U;
        const std::uint32_t high=packed>>4;
        const std::uint32_t left=
            (original_color0_mask[low]&c0)|(original_color1_mask[low]&c1);
        const std::uint32_t right=
            (original_color0_mask[high]&c0)|(original_color1_mask[high]&c1);
        auto* row=out+std::ptrdiff_t(y)*stride;
        std::memcpy(row,&left,4);
        std::memcpy(row+4,&right,4);
    }
}


// Mode 1 / submode 6: original 16x16 single-color pixels.
extern "C" void bink_video_fill16x16(std::uint8_t* out,
                                       std::int32_t stride,
                                       std::uint8_t color) {
    for (int y=0;y<16;++y)
        std::memset(out+std::ptrdiff_t(y)*stride,color,16);
}

// Mode 1 / submode 8: original 16x16 2-color block. Each input pattern
// byte encodes four 2-bit selectors for four consecutive 32-bit pixel
// words. Each decoded 16-pixel row is duplicated vertically to two rows.
// These masks are exact original BINKCONS entries (RVA 0x5A698/0x5A6A8).
extern "C" void bink_video_two_color16x16(std::uint8_t* out,
                                            std::int32_t stride,
                                            std::uint8_t color0,
                                            std::uint8_t color1,
                                            const std::uint8_t* pattern) {
    constexpr std::array<std::uint32_t,4> masks0{
        0xffffffffU,0xffff0000U,0x0000ffffU,0U
    };
    constexpr std::array<std::uint32_t,4> masks1{
        0U,0x0000ffffU,0xffff0000U,0xffffffffU
    };
    const std::uint32_t c0=repeated(color0);
    const std::uint32_t c1=repeated(color1);
    for (int y=0;y<8;++y) {
        const std::uint8_t packed=pattern[y];
        auto* top=out+std::ptrdiff_t(2*y)*stride;
        auto* bottom=top+stride;
        for (int group=0;group<4;++group) {
            const std::uint32_t selection=(packed>>(2*group))&3U;
            const std::uint32_t pixels=
                (masks0[selection]&c0)|(masks1[selection]&c1);
            std::memcpy(top+4*group,&pixels,4);
            std::memcpy(bottom+4*group,&pixels,4);
        }
    }
}
