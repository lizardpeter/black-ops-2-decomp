// Standalone functional tests only. Differential tests against original DLL pending.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "bink_leaf.cpp"

int main() {
    assert(BinkGetError() == t6_bink::g_error);
    BinkSetError("hello");
    assert(std::strcmp(BinkGetError(), "hello") == 0);
    BinkSetError("");
    assert(std::strcmp(BinkGetError(), "") == 0);
    char long_string[301];
    for (int i = 0; i < 300; ++i) long_string[i] = 'a';
    long_string[300] = 0;
    BinkSetError(long_string);
    assert(std::strlen(BinkGetError()) == 254);
    assert(BinkGetError()[254] == 0);

    BinkSetFrameRate(24000, 1001);
    assert(t6_bink::g_frame_rate_numerator == 24000);
    assert(t6_bink::g_frame_rate_denominator == 1001);
    BinkSetFileOffset(UINT64_C(0x123456789ABCDEFF));
    assert(t6_bink::g_file_offset == UINT64_C(0x123456789ABCDEFF));
    BinkSetSimulate(123456);
    assert(t6_bink::g_simulate == 123456);

    alignas(void*) std::uint8_t handle[0x200] = {};
    auto flags = reinterpret_cast<std::uint32_t*>(handle + 0x20);
    *flags = 0x1234;
    BinkSetWillLoop(handle, 1);
    assert(*flags == (0x1234u | 0x80u));
    BinkSetWillLoop(handle, 0);
    assert(*flags == (0x1234u & ~0x80u));
    BinkSetWillLoop(nullptr, 1);

    assert(BinkSetVideoOnOff(nullptr, 5) == 5);
    assert(BinkSetVideoOnOff(handle, 7) == 7);
    assert(*reinterpret_cast<std::uint32_t*>(handle + 0x134) == 7);
    std::uint32_t ids[]{30,40,50}, kinds[]{1,2,3};
    *reinterpret_cast<const std::uint32_t**>(handle + 0x1D0) = ids;
    *reinterpret_cast<const std::uint32_t**>(handle + 0x1C8) = kinds;
    assert(BinkGetTrackID(handle, 1) == 40);
    assert(BinkGetTrackType(handle, 2) == 3);
    assert(BinkGetTrackID(nullptr, 1) == 0);
    assert(BinkGetTrackType(nullptr, 1) == 0);
    return 0;
}
