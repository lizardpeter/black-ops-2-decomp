// Reconstruct FUN_18002F630 from the original Microsoft Store Bink x64 DLL.
// Original binkw64.dll SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Original Ghidra pseudocode and disassembly show this function runs the same
// quantized 8x8 integer inverse transform as FUN_18002F2E0 (already reconstructed
// and bit-for-bit differentially tested), then replicates every resulting byte
// horizontally and vertically. The original writes four 32-bit pairs per output
// row, to both the current and following scanline, and advances by 2 * stride.
//
// This source expresses that exact computation by reusing OUR decompiled
// inverse kernel. No external decoder, library, or proprietary source is used.
// Caller must provide 16x16 output with stride >= 16, plus 64 coefficients and
// 64 quantization factors; this intentionally omits non-original null checks.
#include <cstddef>
#include <cstdint>

extern "C" void bink_video_inverse8x8(std::uint8_t* out, std::int32_t stride,
                                       const std::int16_t* coeff,
                                       const std::int32_t* quant);

extern "C" void bink_video_inverse8x8_2x(std::uint8_t* out, std::int32_t stride,
                                          const std::int16_t* coeff,
                                          const std::int32_t* quant) {
    std::uint8_t unscaled[64];
    bink_video_inverse8x8(unscaled, 8, coeff, quant);
    for (std::int32_t y = 0; y < 8; ++y) {
        std::uint8_t* first = out + static_cast<std::ptrdiff_t>(2 * y) * stride;
        std::uint8_t* second = first + stride;
        for (std::int32_t x = 0; x < 8; ++x) {
            const std::uint8_t v = unscaled[y * 8 + x];
            first[2 * x] = v;
            first[2 * x + 1] = v;
            second[2 * x] = v;
            second[2 * x + 1] = v;
        }
    }
}
