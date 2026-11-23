// Original Bink first-keyframe frame-mode indexed intra transform selector.
// The original 0x252C0 frame coordinator calls original FUN_18002F2E0
// with quantization pointer BINKCONS RVA 0x5B120 + (index & 0xf)*0x100.
// FUN_18002F2E0 was already independently reconstructed and compared
// instruction-for-instruction (as bink_video_inverse8x8); this adapter
// restores the exact caller-side table selection on valid 4-bit indices.
// No FFmpeg, external decoder, or compatibility shortcut.
#include "original_intra_quant8x8.h"
#include <cstdint>

extern "C" void bink_video_inverse8x8(std::uint8_t* out,
    std::int32_t stride, const std::int16_t* coeff, const std::int32_t* quant);

extern "C" void bink_video_inverse8x8_indexed(std::uint8_t* out,
    std::int32_t stride, const std::int16_t* coeff, std::uint32_t index) {
    const std::int32_t* quant=
        t6_bink::original_tables::intra_quant8x8.data()+64*(index & 0xfU);
    bink_video_inverse8x8(out,stride,coeff,quant);
}
