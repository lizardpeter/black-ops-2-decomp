// Source-only checks for XCurl's decompiled getinfo field selector.
// The original-versus-source oracle is tests/dlls/differential_xcurl_getinfo.py.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include "getinfo_unlocked.cpp"

static void set32(void* p, unsigned off, std::uint32_t v) {
    std::memcpy(static_cast<std::uint8_t*>(p) + off, &v, 4);
}
static void set64(void* p, unsigned off, std::uint64_t v) {
    std::memcpy(static_cast<std::uint8_t*>(p) + off, &v, 8);
}
int main() {
    std::array<std::uint8_t, 0x240> handle{};
    for (const auto [off, value] : std::array<std::pair<unsigned, std::uint64_t>, 6>{{
        {0x208, 0xAABBCCDDEEFF0011ULL}, {0x218, 0x0102030405060708ULL},
        {0x148, 0x1122334455667788ULL}, {0x210, 0x8877665544332211ULL},
        {0x1F8, 1234567890123456ULL}, {0x200, 0xFFFFFFFFFFFFFE01ULL}
    }}) set64(handle.data(), off, value);
    for (const auto [off, value] : std::array<std::pair<unsigned, std::uint32_t>, 6>{{
        {0x1E0, 12345}, {0x1F0, 23456}, {0x224, 34567},
        {0x228, 45678}, {0x1DC, 56789}, {0x1D8, 0xFFFFFFFF}
    }}) set32(handle.data(), off, value);
    auto call = [&](unsigned opt, std::uint64_t expected, unsigned width = 8) {
        std::uint64_t out = 0xA5A5A5A5A5A5A5A5ULL;
        void* dst = &out;
        assert(xcurl_easy_getinfo_unlocked(handle.data(), opt, &dst) == 0);
        if (width == 4) {
            assert(std::uint32_t(out) == std::uint32_t(expected));
            assert((out >> 32) == 0xA5A5A5A5U);
        } else assert(out == expected);
    };
    call(0x100001, 0xAABBCCDDEEFF0011ULL);
    call(0x100012, 0x0102030405060708ULL);
    call(0x100015, 0x1122334455667788ULL);
    call(0x100031, 0x8877665544332211ULL);
    call(0x200002, 12345, 4);
    call(0x20000B, 23456, 4);
    call(0x20000D, 34567, 4);
    call(0x200019, 45678, 4);
    call(0x20002E, 56789, 4);
    call(0x200030, 0xFFFFFFFF, 4);
    call(0x600008, 1234567890123456ULL);
    call(0x60000F, 1234567890123456ULL);
    call(0x600010, 0xFFFFFFFFFFFFFE01ULL);
    call(0x600039, UINT64_MAX);
    double expected = static_cast<double>(std::int64_t(1234567890123456LL));
    std::uint64_t raw;
    std::memcpy(&raw, &expected, 8);
    call(0x300008, raw);
    call(0x30000F, raw);
    expected = static_cast<double>(-511LL);
    std::memcpy(&raw, &expected, 8);
    call(0x300010, raw);
    std::uint64_t out = 0xA5A5A5A5A5A5A5A5ULL;
    void* dst = &out;
    assert(xcurl_easy_getinfo_unlocked(handle.data(), 0xDEAD, &dst) == 0x30);
    assert(out == 0xA5A5A5A5A5A5A5A5ULL);
}
