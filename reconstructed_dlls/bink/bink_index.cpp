// Video-only Bink frame index extraction from original Store Bink data.
// Header checks reconstructed from FUN_180017970, plus validated index
// layout from the original 0x2C-byte header and real BO2 BIKi assets.
// This is a deliberately limited, read-only container helper: NOT Bink
// video/audio decompression and NOT a replacement for BinkOpen.
#include "bink_index.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace t6_bink {
static std::uint32_t index_le32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

bool read_video_only_index(const std::uint8_t* data, std::size_t size,
                           FrameSpan* output, std::size_t capacity,
                           HeaderPrefix* parsed) {
    HeaderPrefix h{};
    if (!validate_header_prefix(data, size, &h)) return false;
    // Audio-track descriptors require another 12 bytes per track in this
    // revision; their handling is intentionally left for later reconstruction.
    if (h.audio_track_count != 0 || h.frame_count != h.frame_index_count) return false;
    if (!output || capacity < h.frame_count ||
        size > std::numeric_limits<std::uint32_t>::max()) return false;

    const std::size_t table_end = 44U + 4U * (std::size_t(h.frame_count) + 1U);
    if (table_end > size) return false;
    auto entry = [&](std::size_t i) {
        return index_le32(data + 44U + i * 4U);
    };
    std::uint32_t previous = entry(0) & ~1U;
    if (previous < table_end) return false;
    for (std::size_t i = 0; i < h.frame_count; ++i) {
        const std::uint32_t raw = entry(i);
        const std::uint32_t next = entry(i + 1) & ~1U;
        const std::uint32_t start = raw & ~1U;
        if (start < table_end || start < previous || next < start || next > size)
            return false;
        output[i] = FrameSpan{start, next, (raw & 1U) != 0};
        previous = next;
    }
    if (parsed) *parsed = h;
    return true;
}
} // namespace t6_bink
