// Reconstructed pixel-prediction arithmetic from original Bink video routine
// FUN_18002F9F0 (RVA 0x2F9F0) in pinned Store binkw64.dll.
// SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// The original routine looks up 64 signed 32-bit quantization factors at
// BINKCONS RVA 0x5C120 + quant_index*0x100, executes the same decompiled 8x8
// inverse transform as FUN_18002F2E0, then adds each resulting byte modulo
// 256 to the corresponding pixel from a reference frame.
//
// This source isolates that ENTIRE arithmetic stage, taking the original
// quantization row explicitly rather than silently embedding an incomplete
// index-dependent BINKCONS table. It is not yet an ABI-equivalent implementation
// of FUN_18002F9F0: restoring the original table/index boundary is still
// required. No replacement codec source or third-party decoder is used.
#include <cstddef>
#include <cstdint>

extern "C" void bink_video_inverse8x8(std::uint8_t* out, std::int32_t stride,
                                       const std::int16_t* coeff,
                                       const std::int32_t* quant);

extern "C" void bink_video_predict8x8_core(std::uint8_t* out, std::int32_t stride,
                                            const std::int16_t* coeff,
                                            const std::int32_t* quant,
                                            const std::uint8_t* reference,
                                            std::int32_t reference_stride) {
    std::uint8_t residual[64];
    bink_video_inverse8x8(residual, 8, coeff, quant);
    for (std::int32_t y = 0; y < 8; ++y) {
        std::uint8_t* dest = out + static_cast<std::ptrdiff_t>(y) * stride;
        const std::uint8_t* pred =
            reference + static_cast<std::ptrdiff_t>(y) * reference_stride;
        for (std::int32_t x = 0; x < 8; ++x)
            dest[x] = static_cast<std::uint8_t>(
                unsigned(pred[x]) + unsigned(residual[y * 8 + x]));
    }
}
