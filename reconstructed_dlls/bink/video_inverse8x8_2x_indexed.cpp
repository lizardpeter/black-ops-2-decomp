// Restores original FUN_1800252C0 mode-1/submode-5 quant-table selection:
// original BINKCONS RVA 0x5B120 + (index & 0xF)*256.
// Underlying 2x inverse transform already reconstructed directly from
// original FUN_18002F630 at RVA 0x2F630 and originally validated.
// Tables are the exact original Store binkw64.dll bytes, not an SDK
// dependency, external decoder or approximation.
#include "original_intra_quant8x8.h"
#include <cstdint>

extern "C" void bink_video_inverse8x8_2x(std::uint8_t*,std::int32_t,
    const std::int16_t*,const std::int32_t*);

extern "C" void bink_video_inverse8x8_2x_indexed(
        std::uint8_t* out,std::int32_t stride,
        const std::int16_t* coefficients,std::uint32_t index) {
    const std::int32_t* table=
        t6_bink::original_tables::intra_quant8x8.data()
        +64*(index&0xfU);
    bink_video_inverse8x8_2x(out,stride,coefficients,table);
}
