// Predicate tests from two real BO2 BIKi headers and negative cases.
// Does not claim to validate entire Bink streams or decode a single frame.
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include "header_validation.cpp"

using Words = std::array<std::uint32_t, 11>;
static std::array<std::uint8_t, 44> encode(const Words& words) {
    std::array<std::uint8_t, 44> bytes{};
    for (std::size_t i = 0; i < 11; ++i) {
        auto v = words[i];
        for (unsigned j = 0; j < 4; ++j) bytes[4 * i + j] = std::uint8_t(v >> (j * 8));
    }
    return bytes;
}
static bool accepts(const Words& words) {
    const auto bytes = encode(words);
    t6_bink::HeaderPrefix parsed{};
    return t6_bink::validate_header_prefix(bytes.data(), bytes.size(), &parsed);
}

int main() {
    // Header fields recovered from actual R2 assets, not generated Bink data.
    const Words la_seq_6 = {
        0x694B4942U, 335156, 150, 3384, 150, 256, 256, 30, 1, 0, 0
    };
    const Words karma_security = {
        0x694B4942U, 422740, 114, 8044, 114, 256, 256, 10000000, 333333, 0, 0
    };
    assert(accepts(la_seq_6));
    assert(accepts(karma_security));
    auto bytes = encode(la_seq_6);
    t6_bink::HeaderPrefix parsed{};
    assert(t6_bink::validate_header_prefix(bytes.data(), bytes.size(), &parsed));
    assert(parsed.frame_count == 150 && parsed.width == 256 && parsed.height == 256);
    assert(parsed.rate_numerator == 30 && parsed.rate_denominator == 1);
    assert(!t6_bink::validate_header_prefix(bytes.data(), 43, &parsed));
    assert(!t6_bink::validate_header_prefix(nullptr, 44, &parsed));
    assert(!t6_bink::validate_header_prefix(bytes.data(), 44, nullptr));

    for (auto magic : {0x664B4942U, 0x674B4942U, 0x684B4942U, 0x694B4942U}) {
        Words x = la_seq_6;
        x[0] = magic;
        assert(accepts(x));
    }
    Words x = la_seq_6;
    x[0] = 0x6A4B4942U; assert(!accepts(x)); // BIKj
    // Values numerically between BIKf and BIKi are NOT necessarily valid.
    x = la_seq_6; x[0] = 0x664B4943U; assert(!accepts(x)); // BJKf
    x = la_seq_6; x[0] = 0x674B4941U; assert(!accepts(x)); // BIKe
    x = la_seq_6; x[0] = 0; assert(!accepts(x));
    x = la_seq_6; x[2] = 0; assert(!accepts(x)); // zero frames
    x = la_seq_6; x[2] = 151; assert(!accepts(x)); // frame count > index count
    x = la_seq_6; x[4] = 1000001; assert(!accepts(x));
    x = la_seq_6; x[3] = 335157; assert(!accepts(x)); // largest frame > payload
    x = la_seq_6; x[10] = 257; assert(!accepts(x));
    x = la_seq_6; x[1] = 16; x[3] = 0; assert(!accepts(x)); // offset table too big
    x = la_seq_6; x[5] = 32768; assert(!accepts(x));
    x = la_seq_6; x[6] = 32768; assert(!accepts(x));
    x = la_seq_6; x[7] = 0; assert(!accepts(x));
    x = la_seq_6; x[8] = 0; assert(!accepts(x));
}
