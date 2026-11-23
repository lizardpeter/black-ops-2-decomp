// Full six-argument native transcription of original Bink FUN_18002F9F0,
// RVA 0x2F9F0, from SHA256-pinned Store binkw64.dll:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// The original selects one of sixteen 8x8 int32 quantization matrices at
// RVA 0x5C120 + quant_index*0x100, applies the original two-pass inverse
// transform, and adds output bytes to the reference pixels modulo 256.
// This translation composes our already-original-verified transform+prediction
// core with the exact 16 original BINKCONS matrices, WITHOUT any replacement
// decoder or third-party media libraries.
//
// Fidelity preconditions: quant_index in [0,15] as on valid Bink streams,
// valid 8x8 coefficient input and reference/output pixel planes; strides
// sufficient for 8 pixels per row. Caller handles frame allocation.
#include "original_quant8x8.h"
#include <cstdint>

extern "C" void bink_video_predict8x8_core(std::uint8_t* out,
    std::int32_t stride, const std::int16_t* coeff,
    const std::int32_t* quant, const std::uint8_t* reference,
    std::int32_t reference_stride);

extern "C" void bink_video_predict8x8(std::uint8_t* out,
    std::int32_t stride, const std::int16_t* coeff,
    std::int32_t quant_index, const std::uint8_t* reference,
    std::int32_t reference_stride) {
    const std::int32_t* matrix =
        t6_bink::original_tables::quant8x8.data() + 64 * quant_index;
    bink_video_predict8x8_core(out, stride, coeff, matrix, reference,
                               reference_stride);
}
