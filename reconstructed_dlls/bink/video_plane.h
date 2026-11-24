// Shared ABI for the original-machine-derived Bink plane research decoder.
// Caller supplies trusted compressed packet and independent destination and
// previous-frame buffers. This is not a hardened public bitstream parser.
#pragma once
#include <cstdint>

struct BinkFirstLumaTrace {
    std::uint32_t status;
    std::uint32_t error_x;
    std::uint32_t error_y;
    std::uint32_t mode;
    std::uint32_t submode;
    std::uint32_t type_counts[10];
    std::uint32_t submode_counts[10];
    std::uint32_t input_words;
    std::uint32_t input_bits_available;
    std::uint32_t input_cache;
    std::uint32_t first_mode_count;
    std::uint8_t first_mode_bytes[32];
    std::uint32_t first_submode_count;
    std::uint8_t first_submode_bytes[32];
};

extern "C" int bink_research_decode_video_plane(
    const std::uint8_t* compressed,std::uint32_t compressed_bytes,
    std::uint32_t start,std::uint32_t split,
    std::uint32_t width,std::uint32_t height,
    std::uint8_t* output,std::uint32_t stride,
    const std::uint8_t* reference,BinkFirstLumaTrace* report);
