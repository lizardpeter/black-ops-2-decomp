#pragma once
#include <cstddef>
#include <cstdint>
#include "header_validation.h"

namespace t6_bink {
struct FrameSpan {
    std::uint32_t start;
    std::uint32_t end;
    bool keyframe;
};

// Safe analysis helper derived from original Bink header/index layout.
// Supports streams with no audio track records and equal frame/index counts.
// No decompression is performed.
bool read_video_only_index(const std::uint8_t* data, std::size_t size,
                           FrameSpan* output, std::size_t capacity,
                           HeaderPrefix* parsed = nullptr);
}
