// Standalone tests for the original-code reconstruction at Bink RVA 0x17870.
// Not a substitute for differential execution against the pinned original.
#include <cassert>
#include <cstdint>
#include <cstring>
#include "bink_frames.cpp"

static void store32(void* p, std::size_t offset, std::uint32_t value) {
    std::memcpy(static_cast<std::uint8_t*>(p) + offset, &value, sizeof(value));
}

int main() {
    alignas(8) std::uint8_t handle[0x200] = {};
    alignas(8) std::uint32_t out[54] = {};
    store32(handle, 0x1B8, 1921);
    store32(handle, 0x1BC, 1081);
    store32(handle, 0x20, 0x100000);
    store32(handle, 0x08, 1);
    std::memset(out, 0xA5, sizeof(out));
    BinkGetFrameBuffersInfo(handle, out);
    assert(out[0] == 1);
    assert(out[1] == 1928 && out[2] == 1088);
    assert(out[3] == 968 && out[4] == 544);
    assert(out[5] == 0);
    assert(out[6] == 1 && out[12] == 1 && out[18] == 1);
    for (unsigned i = 8; i <= 26; i += 6) {
        assert(out[i] == 0 && out[i + 1] == 0);
    }
    assert(out[10] == 0 && out[16] == 0 && out[22] == 0 && out[28] == 0);
    assert(out[24] == 1);
    assert(out[30] == 0xA5A5A5A5); // single-frame path must not copy second descriptor

    store32(handle, 0x08, 2);
    std::memset(out, 0xA5, sizeof(out));
    BinkGetFrameBuffersInfo(handle, out);
    assert(out[0] == 2);
    assert(std::memcmp(out + 30, out + 6, 0x60) == 0);

    std::memset(out, 0xA5, sizeof(out));
    BinkGetFrameBuffersInfo(nullptr, out);
    for (auto x : out) assert(x == 0xA5A5A5A5);
    BinkGetFrameBuffersInfo(handle, nullptr);
    return 0;
}
