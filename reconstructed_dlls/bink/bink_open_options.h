#pragma once
#include <cstddef>
#include <cstdint>
namespace t6_bink {
struct OpenOptions {
    std::uint64_t file_offset;          // +0x00
    std::uint32_t frame_rate_number;    // +0x08
    std::uint32_t frame_rate_denominator;// +0x0C
    std::uint32_t io_size;              // +0x10
    std::uint32_t simulate;             // +0x14
    std::uint32_t track_count;          // +0x18
    std::uint32_t gap;                  // +0x1C, original stack uninitialized
    const std::uint32_t* track_ids;     // +0x20
    std::uint64_t secondary_offset;     // +0x28
};
static_assert(sizeof(OpenOptions) == 0x30);
static_assert(offsetof(OpenOptions, track_ids) == 0x20);

}
