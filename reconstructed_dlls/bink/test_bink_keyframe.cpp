// Unit tests for the original BinkGetKeyFrame 1-based frame seek logic.
// Original-versus-reconstruction differential tests live in tests/dlls/.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "bink_keyframe.cpp"

int main() {
    alignas(8) std::uint8_t handle[0x200] = {};
    std::uint32_t offsets[11] = {
        1, 100, 200, 300, 401, 500, 600, 700, 801, 900, 1000
    };
    std::uint32_t count = 10;
    std::memcpy(handle + 8, &count, sizeof(count));
    const std::uint32_t* ptr = offsets;
    std::memcpy(handle + 0x1A0, &ptr, sizeof(ptr));

    assert(BinkGetKeyFrame(nullptr, 1, 0) == 0);
    assert(BinkGetKeyFrame(handle, 1, 0) == 1);
    assert(BinkGetKeyFrame(handle, 2, 0) == 1);
    assert(BinkGetKeyFrame(handle, 5, 0) == 5);
    assert(BinkGetKeyFrame(handle, 7, 0) == 5);
    assert(BinkGetKeyFrame(handle, 2, 1) == 5);
    assert(BinkGetKeyFrame(handle, 6, 1) == 9);
    assert(BinkGetKeyFrame(handle, 9, 1) == 9);
    assert(BinkGetKeyFrame(handle, 9, 0x81) == 0);
    assert(BinkGetKeyFrame(handle, 7, 2) == 5);
    assert(BinkGetKeyFrame(handle, 8, 2) == 9);
    assert(BinkGetKeyFrame(handle, 9, 0x82) == 5);
    assert(BinkGetKeyFrame(handle, 3, 3) == 0);
    assert(BinkGetKeyFrame(handle, 9, 3) == 9);
    assert(BinkGetKeyFrame(handle, 9, 0x83) == 0);
}
