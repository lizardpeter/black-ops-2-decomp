// Source-only regression tests for two original Bink sample permutations.
// Pinned-original-versus-C++ differentials are in tests/dlls/.
#include <array>
#include <cassert>
#include <cstdint>
#include "audio_permute32.cpp"

int main() {
    std::array<std::uint32_t, 32> a{}, b{};
    for (std::uint32_t i = 0; i < 32; ++i) {
        a[i] = i;
        b[i] = i;
    }
    t6_bink::internal::bink_audio_permute32_a(a.data());
    const std::uint32_t a_expected[32] = {
        0,1,16,17,8,9,24,25,4,5,20,21,12,13,28,29,
        2,3,18,19,10,11,26,27,6,7,22,23,14,15,30,31
    };
    for (std::uint32_t i = 0; i < 32; ++i) assert(a[i] == a_expected[i]);
    t6_bink::internal::bink_audio_permute32_b(b.data());
    const std::uint32_t b_expected[32] = {
        0,1,30,31,14,15,22,23,6,7,26,27,10,11,18,19,
        2,3,28,29,12,13,20,21,4,5,24,25,8,9,16,17
    };
    for (std::uint32_t i = 0; i < 32; ++i) assert(b[i] == b_expected[i]);
    // Reordering raw 32-bit words preserves all payload bits, including NaNs.
    std::array<std::uint32_t, 32> unusual{};
    for (std::uint32_t i = 0; i < 32; ++i) unusual[i] = 0x7fc00000U + i;
    auto old = unusual;
    t6_bink::internal::bink_audio_permute32_b(unusual.data());
    for (std::uint32_t i = 0; i < 32; ++i) assert(unusual[i] == old[b_expected[i]]);
}
