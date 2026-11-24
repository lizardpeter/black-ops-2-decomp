#pragma once
#include <cstdint>
#include <span>

namespace t6_bink {
// Store BIKi packet routing transcribed from original FUN_1800274A0
// (alpha call at RVA 0x277D2, Y at 0x27882, chroma at 0x27941/0x279BF).
// These checks are an added source-side contract; not an untrusted-media
// guarantee for the underlying original-derived entropy decoder.
inline constexpr std::uint32_t kAlphaFlag=0x100000;
struct VideoPacketLayout {
    std::uint32_t alpha_start=0,alpha_end=0;
    std::uint32_t luma_start=0,luma_end=0;
    std::uint32_t end=0;
};
inline std::uint32_t packet_le32(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
        (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
inline bool parse_video_packet(std::span<const std::uint8_t> data,
                               std::uint32_t flags,VideoPacketLayout& out) {
    if((flags&~kAlphaFlag) || data.size()<8 ||
       data.size()>262144 || (data.size()&3U))return false;
    VideoPacketLayout layout{};
    layout.end=static_cast<std::uint32_t>(data.size());
    std::uint32_t luma_word=0;
    if(flags&kAlphaFlag) {
        layout.alpha_start=4;
        layout.alpha_end=packet_le32(data.data());
        if(layout.alpha_end<=4 || (layout.alpha_end&3U) ||
           layout.alpha_end>=layout.end-4)return false;
        luma_word=layout.alpha_end;
    }
    layout.luma_start=luma_word+4;
    const auto relative=packet_le32(data.data()+luma_word);
    if(relative<=4 || (relative&3U) || relative>=layout.end-luma_word)
        return false;
    layout.luma_end=luma_word+relative;
    out=layout;
    return true;
}
}
