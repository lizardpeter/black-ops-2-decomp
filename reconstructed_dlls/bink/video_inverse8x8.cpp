// Original BO2 Store binkw64.dll video block reconstruction.
// FUN_18002F2E0 (RVA 0x2F2E0), reached from the Bink video decompression path.
// SHA256 original DLL:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Faithful two-pass 8x8 inverse transform, using ORIGINAL x64 integer
// constants, signed shifts and 32-bit wrapping. This is a source transcription
// of original machine behavior, not a wrapper for any decoder or codec.
// One block only: the container, entropy decoding, frame traversal and Bink
// video output pipeline remain unreconstructed.
//
// Original arguments: output pointer; signed row stride; 8x8 int16 coefficients
// (row-major); 8x8 int32 quantization factors (row-major).
// The game supplies valid, non-overlapping buffers. Negative strides must
// point to a suitably positioned output buffer.
#include <bit>
#include <cstddef>
#include <initializer_list>
#include <cstdint>

namespace t6_bink::internal {
static std::int32_t add32(std::int32_t a, std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::uint32_t(a) + std::uint32_t(b));
}
static std::int32_t sub32(std::int32_t a, std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::uint32_t(a) - std::uint32_t(b));
}
static std::int32_t mul32(std::int32_t a, std::int32_t b) {
    return std::bit_cast<std::int32_t>(std::uint32_t(a) * std::uint32_t(b));
}
static std::int32_t mulshift11(std::int32_t a, std::int32_t b) {
    return mul32(a, b) >> 11;
}
static std::uint8_t pixel(std::int32_t value) {
    // Original instruction: (uint32)value >> 8, truncate to 8 bits.
    return static_cast<std::uint8_t>(std::uint32_t(value) >> 8);
}
} // namespace t6_bink::internal

extern "C" void bink_video_inverse8x8(std::uint8_t* out, std::int32_t stride,
                                      const std::int16_t* coeff,
                                      const std::int32_t* quant) {
    using namespace t6_bink::internal;
    std::int32_t temp[64]{};
    for (unsigned col = 0; col < 8; ++col) {
        auto c = [&](unsigned row) -> std::int32_t { return coeff[row * 8 + col]; };
        auto q = [&](unsigned row) -> std::int32_t { return quant[row * 8 + col]; };
        std::int32_t a1 = 0;
        if (c(1) == 0 && c(2) == 0 && c(3) == 0 && c(4) == 0 &&
            c(5) == 0 && c(6) == 0 && c(7) == 0) {
            a1 = mulshift11(c(0), q(0));
            for (unsigned row : {0U, 1U, 2U, 3U, 5U, 6U, 7U})
                temp[row * 8 + col] = a1;
        } else {
            std::int32_t a8 = mulshift11(c(0), q(0));
            std::int32_t a4 = mulshift11(c(2), q(2));
            std::int32_t a3 = mulshift11(c(6), q(6));
            std::int32_t a2 = mulshift11(c(4), q(4));
            a1 = add32(a3, a4);
            std::int32_t a10 = add32(a2, a8);
            a8 = sub32(a8, a2);
            std::int32_t a11 = add32(a1, a10);
            a10 = sub32(a10, a1);
            a1 = sub32(mulshift11(sub32(a4, a3), 0xB50), a1);
            a3 = add32(a1, a8);
            a8 = sub32(a8, a1);
            std::int32_t a7 = mulshift11(c(1), q(1));
            a2 = mulshift11(c(3), q(3));
            std::int32_t a12 = mulshift11(c(5), q(5));
            a1 = add32(a12, a2);
            a12 = sub32(a12, a2);
            a4 = mulshift11(c(7), q(7));
            a2 = add32(a4, a7);
            a7 = sub32(a7, a4);
            std::int32_t a5 = add32(a2, a1);
            a4 = mulshift11(add32(a7, a12), 0xEC8);
            temp[0 * 8 + col] = add32(a5, a11);
            std::int32_t a9 = add32(sub32(mulshift11(a12, -0x14E8), a5), a4);
            a12 = sub32(mulshift11(sub32(a2, a1), 0xB50), a9);
            temp[1 * 8 + col] = add32(a9, a3);
            a2 = add32(sub32(mulshift11(a7, 0x8A9), a4), a12);
            temp[2 * 8 + col] = add32(a12, a8);
            a1 = add32(a2, a10);
            temp[7 * 8 + col] = sub32(a11, a5);
            temp[6 * 8 + col] = sub32(a3, a9);
            temp[5 * 8 + col] = sub32(a8, a12);
            temp[3 * 8 + col] = sub32(a10, a2);
        }
        temp[4 * 8 + col] = a1;
    }

    for (unsigned row = 0; row < 8; ++row) {
        const std::int32_t* p = temp + row * 8 + 6;
        std::int32_t a14 = add32(p[-6], p[-2]);
        std::int32_t a7 = sub32(p[-6], p[-2]);
        std::int32_t a1 = add32(p[0], p[-4]);
        std::int32_t a12 = add32(a1, a14);
        a14 = sub32(a14, a1);
        a1 = sub32(mulshift11(sub32(p[-4], p[0]), 0xB50), a1);
        std::int32_t a10 = add32(p[-3], p[-1]);
        std::int32_t a8 = sub32(p[-1], p[-3]);
        std::int32_t a3 = add32(a1, a7);
        a7 = sub32(a7, a1);
        a1 = add32(p[-5], p[1]);
        std::int32_t a5 = sub32(p[-5], p[1]);
        std::int32_t a4 = add32(a1, a10);
        std::int32_t a2 = mulshift11(add32(a5, a8), 0xEC8);
        a8 = add32(sub32(mulshift11(a8, -0x14E8), a4), a2);
        a10 = sub32(mulshift11(sub32(a1, a10), 0xB50), a8);
        a1 = add32(sub32(mulshift11(a5, 0x8A9), a2), a10);
        std::uint8_t* result = out + static_cast<std::ptrdiff_t>(row) * stride;
        result[0] = pixel(add32(add32(a4, 0x7F), a12));
        result[1] = pixel(add32(add32(a8, 0x7F), a3));
        result[2] = pixel(add32(add32(a10, 0x7F), a7));
        result[7] = pixel(add32(sub32(a12, a4), 0x7F));
        result[6] = pixel(add32(sub32(a3, a8), 0x7F));
        result[5] = pixel(add32(sub32(a7, a10), 0x7F));
        result[3] = pixel(add32(sub32(a14, a1), 0x7F));
        result[4] = pixel(add32(add32(a1, 0x7F), a14));
    }
}
