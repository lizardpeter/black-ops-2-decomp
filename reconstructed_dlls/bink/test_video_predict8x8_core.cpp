// Source-level tests for Bink 8x8 inverse transform plus prediction.
// Pinned-original differential: tests/dlls/differential_video_predict8x8.py.
#include <array>
#include <cassert>
#include <cstdint>
#include "video_inverse8x8.cpp"
#include "video_predict8x8_core.cpp"
#include "video_predict8x8.cpp"

int main() {
    std::array<std::int16_t, 64> coefficients{};
    std::array<std::int32_t, 64> quant{};
    quant.fill(2048);
    for (int stride : {8, 16, 24}) {
        std::array<std::uint8_t, 192> prediction{};
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < stride; ++x)
                prediction[y * stride + x] =
                    static_cast<std::uint8_t>(x + y * 19);
        std::array<std::uint8_t, 192> output{};
        output.fill(0xA5);
        bink_video_predict8x8_core(output.data(), stride, coefficients.data(),
                                    quant.data(), prediction.data(), stride);
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 8; ++x)
                assert(output[y * stride + x] == prediction[y * stride + x]);
            for (int x = 8; x < stride; ++x)
                assert(output[y * stride + x] == 0xA5);
        }
    }

    coefficients[0] = 2048;
    coefficients[9] = -1024;
    std::array<std::uint8_t, 64> prediction{}, expected{}, result{};
    prediction.fill(250);
    bink_video_inverse8x8(expected.data(), 8, coefficients.data(), quant.data());
    for (auto& x : expected) x = static_cast<std::uint8_t>(x + 250);
    bink_video_predict8x8_core(result.data(), 8, coefficients.data(), quant.data(),
                                prediction.data(), 8);
    assert(result == expected);
    // Complete original six-argument entry: for every original BINKCONS
    // matrix, the indexed wrapper must equal our already verified core.
    for (int idx=0;idx<16;++idx) {
        const auto* matrix = t6_bink::original_tables::quant8x8.data()+64*idx;
        std::array<std::uint8_t,64> via_index{}, via_matrix{};
        bink_video_predict8x8(via_index.data(),8,coefficients.data(),
                              idx,prediction.data(),8);
        bink_video_predict8x8_core(via_matrix.data(),8,coefficients.data(),
                                   matrix,prediction.data(),8);
        assert(via_index == via_matrix);
    }
}
