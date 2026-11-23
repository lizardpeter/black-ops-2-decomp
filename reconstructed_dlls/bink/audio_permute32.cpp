// Two original Bink codec sample-word permutation kernels.
// Original binkw64.dll SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Source reconstructed directly from original Ghidra pseudocode and x64
// instructions, not FFmpeg or any other decoder. The 32-bit transfers are
// bit-preserving even for NaN, infinity and signed-zero sample payloads.
//
// Both routines are part of Bink's original audio transform call graph:
//   FUN_18002A450 -> FUN_1800293C0
//   FUN_18002A570 -> FUN_1800295A0
//
// They require 32 readable/writable 32-bit elements; no external callees.
#include <cstdint>
#include <cstring>

namespace t6_bink::internal {

static void permute32(std::uint32_t* x, const std::uint8_t (&from)[32]) {
    std::uint32_t input[32];
    std::memcpy(input, x, sizeof(input));
    for (unsigned i = 0; i < 32; ++i) {
        x[i] = input[from[i]];
    }
}

// Original RVA 0x2A450: twelve 32-bit word swaps, preserving remaining words.
extern "C" void bink_audio_permute32_a(std::uint32_t* x) {
    constexpr std::uint8_t source[32] = {
        0, 1, 16, 17, 8, 9, 24, 25,
        4, 5, 20, 21, 12, 13, 28, 29,
        2, 3, 18, 19, 10, 11, 26, 27,
        6, 7, 22, 23, 14, 15, 30, 31
    };
    permute32(x, source);
}

// Original RVA 0x2A570: 32-word fixed permutation, no floating arithmetic.
extern "C" void bink_audio_permute32_b(std::uint32_t* x) {
    constexpr std::uint8_t source[32] = {
        0, 1, 30, 31, 14, 15, 22, 23,
        6, 7, 26, 27, 10, 11, 18, 19,
        2, 3, 28, 29, 12, 13, 20, 21,
        4, 5, 24, 25, 8, 9, 16, 17
    };
    permute32(x, source);
}
} // namespace t6_bink::internal
