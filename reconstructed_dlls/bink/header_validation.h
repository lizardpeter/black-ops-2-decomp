#pragma once
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
bool validate_header_prefix(const std::uint8_t* data, std::size_t bytes,
                            HeaderPrefix* parsed);
}
