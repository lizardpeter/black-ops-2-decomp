// Original Bink frame-buffer layout source reconstruction, Store x64 DLL.
// Original binary SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
// RVA 0x17870. No third-party decoder or library implementation used.
// This file reconstructs the *metadata* API. Actual frame decompression is
// elsewhere and remains unreconstructed.
#include <cstdint>
#include <cstring>

namespace t6_bink {
static std::uint32_t read32(const void* handle, std::size_t offset) {
    std::uint32_t result;
    std::memcpy(&result, static_cast<const std::uint8_t*>(handle) + offset, sizeof(result));
    return result;
}
static std::uint32_t round8(std::uint32_t n) {
    return (n + 7U) & ~7U;
}
} // namespace t6_bink

// The output is at least 0xD8 bytes (54 uint32). The caller owns this storage.
// Original performs no output-size validation and does nothing on null pointers.
extern "C" void BinkGetFrameBuffersInfo(const void* handle, void* frame_buffer_info) {
    if (!handle || !frame_buffer_info) return;
    auto* out = static_cast<std::uint32_t*>(frame_buffer_info);
    const std::uint32_t width = t6_bink::read32(handle, 0x1B8);
    const std::uint32_t height = t6_bink::read32(handle, 0x1BC);
    const std::uint32_t flags = t6_bink::read32(handle, 0x20);
    const std::uint32_t frame_count = t6_bink::read32(handle, 0x08);

    out[5] = 0;
    out[1] = t6_bink::round8(width);
    out[2] = t6_bink::round8(height);
    out[3] = t6_bink::round8((width + 1U) >> 1);
    out[4] = t6_bink::round8((height + 1U) >> 1);
    out[6] = 1;
    out[12] = 1;
    out[18] = 1;
    for (unsigned i = 8U; i <= 26U; i += 6U) {
        // Original uses a 64-bit store at each pointer field.
        std::memset(out + i, 0, 8);
    }
    out[10] = 0;
    out[16] = 0;
    out[22] = 0;
    out[28] = 0;
    out[24] = (flags >> 20) & 1U;
    if (frame_count == 1U) {
        out[0] = 1;
    } else {
        out[0] = 2;
        std::memcpy(out + 30, out + 6, 0x60);
    }
}
