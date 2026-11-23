// Faithful reconstruction of the 44-byte Bink header *validation predicate*
// inside original FUN_180017970 (BinkOpen core). Original binkw64.dll:
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// NOT a reconstructed BinkOpen and NOT a codec. This isolates the predicate
// observed in Ghidra after the original I/O layer has read a 0x2C-byte header.
// The explicit length check is an ADDED safe helper contract: the original
// routine had already read 44 bytes and checked its own I/O status.
#include <cstddef>
#include <cstdint>

namespace t6_bink {
struct HeaderPrefix {
    std::uint32_t magic;
    std::uint32_t payload_bytes;
    std::uint32_t frame_count;
    std::uint32_t largest_frame_bytes;
    std::uint32_t frame_index_count;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t rate_numerator;
    std::uint32_t rate_denominator;
    std::uint32_t flags;
    std::uint32_t audio_track_count;
};
static_assert(sizeof(HeaderPrefix) == 44);

static std::uint32_t header_le32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}

// Original stage of FUN_180017970 checks the header after reading 0x2C bytes.
// No assumptions about the validity of frame-offset entries or decoded video
// data are implied by this initial stage.
bool validate_header_prefix(const std::uint8_t* data, std::size_t bytes,
                            HeaderPrefix* parsed) {
    if (!data || bytes < sizeof(HeaderPrefix) || !parsed) return false;
    std::uint32_t words[11] = {};
    for (std::size_t i = 0; i < 11; ++i) words[i] = header_le32(data + 4 * i);
    HeaderPrefix header{words[0], words[1], words[2], words[3], words[4],
                        words[5], words[6], words[7], words[8], words[9], words[10]};

    // Original accepts BIKf, BIKg, BIKh, BIKi (little-endian magic).
    if (header.magic < 0x664B4942U || header.magic > 0x694B4942U) return false;
    if (!header.frame_count) return false;
    if (header.frame_count > header.frame_index_count ||
        header.frame_index_count >= 1000001U) return false;
    if (header.largest_frame_bytes > header.payload_bytes) return false;
    if (header.audio_track_count >= 257U) return false;
    const std::uint64_t index_bytes =
        (std::uint64_t(header.audio_track_count) + 1) *
        header.frame_index_count * 4U;
    if (index_bytes > header.payload_bytes) return false;
    if (header.width >= 32768U || header.height >= 32768U) return false;
    if (!header.rate_numerator || !header.rate_denominator) return false;
    *parsed = header;
    return true;
}
} // namespace t6_bink
