#pragma once
#include "video_packet.h"
#include "video_plane.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace t6_bink {
// Exact x64 metadata layout recovered from original BinkGetFrameBuffersInfo
// (0x17870), BinkRegisterFrameBuffers (0x17950) and coordinator (0x274A0).
// This is a descriptor, NOT the original BINK handle or an export shim.
struct RegisteredPlane {
    std::uint32_t allocate=0;
    std::uint32_t reserved0=0;
    std::uint8_t* data=nullptr;
    std::uint32_t pitch=0;
    std::uint32_t reserved1=0;
};
struct RegisteredSlot {
    std::array<RegisteredPlane,4> planes{};
};
struct RegisteredFrameBuffers {
    std::uint32_t buffer_count=0;
    std::uint32_t y_width=0,y_height=0,c_width=0,c_height=0;
    std::uint32_t frame_num=0;
    std::array<RegisteredSlot,2> slots{};
};
static_assert(sizeof(void*)==8);
static_assert(sizeof(RegisteredPlane)==0x18);
static_assert(offsetof(RegisteredPlane,data)==8);
static_assert(offsetof(RegisteredPlane,pitch)==0x10);
static_assert(sizeof(RegisteredSlot)==0x60);
static_assert(offsetof(RegisteredFrameBuffers,frame_num)==0x14);
static_assert(offsetof(RegisteredFrameBuffers,slots)==0x18);
static_assert(sizeof(RegisteredFrameBuffers)==0xD8);

struct RegisteredDecodeReport {
    std::uint32_t status=0;
    std::uint32_t plane=0;
    std::uint32_t destination_slot=0;
    std::uint32_t next_offset=0;
    BinkFirstLumaTrace trace{};
};

// Source-only coordinator for the trusted Store BIKi packet subset.
// Caller owns, allocates, and keeps all registered planes alive; each
// must contain at least pitch*height accessible bytes. Matching pitches
// are required for both frame slots. The descriptor remains unchanged,
// just as in 0x274A0; caller advances the slot after successful decode.
// Failure can leave partial writes in the destination, which must be
// discarded. This does not claim original coordinator return statistics.
inline int decode_registered_packet(std::span<const std::uint8_t> packet,
                                    std::uint32_t flags,
                                    const RegisteredFrameBuffers& info,
                                    RegisteredDecodeReport& report) {
    report=RegisteredDecodeReport{};
    const auto fail=[&](std::uint32_t code) {
        report.status=code;
        return static_cast<int>(code);
    };
    VideoPacketLayout layout{};
    if(!parse_video_packet(packet,flags,layout))return fail(1);
    const bool alpha=(flags&kAlphaFlag)!=0;
    const unsigned planes=alpha?4U:3U;
    if((info.buffer_count!=1 && info.buffer_count!=2) ||
       info.frame_num>=info.buffer_count ||
       info.y_width<8 || info.y_width>1280 ||
       info.y_height<8 || info.y_height>720 ||
       (info.y_width&7U) || (info.y_height&7U) ||
       info.c_width!=((info.y_width/2+7U)&~7U) ||
       info.c_height!=((info.y_height/2+7U)&~7U))
        return fail(2);
    const unsigned ref=info.frame_num;
    const unsigned dst=info.buffer_count==2 ? ref^1U : ref;
    report.destination_slot=dst;
    for(unsigned p=0;p<planes;++p) {
        const auto w=(p==0 || p==3)?info.y_width:info.c_width;
        const auto& current=info.slots[dst].planes[p];
        const auto& previous=info.slots[ref].planes[p];
        if(!current.data || !previous.data || current.pitch!=previous.pitch ||
           current.pitch<w || current.pitch>16384U ||
           (info.buffer_count==2 && current.data==previous.data)) {
            report.plane=p;
            return fail(2);
        }
    }
    // Original caller supplies the full indexed frame end; coordinator
    // adds four bytes. Verified guard content invariance for pinned clips.
    std::vector<std::uint8_t> padded(packet.begin(),packet.end());
    padded.resize(packet.size()+4,0);
    const auto limit=static_cast<std::uint32_t>(padded.size());
    constexpr std::array<unsigned,4> order{3,0,1,2};
    std::uint32_t pos=alpha?layout.alpha_start:layout.luma_start;
    for(unsigned i=alpha?0U:1U;i<order.size();++i) {
        const unsigned p=order[i];
        report.plane=p;
        if(p==0)pos=layout.luma_start;
        if(p==1)pos=layout.luma_end;
        const auto w=(p==0 || p==3)?info.y_width:info.c_width;
        const auto h=(p==0 || p==3)?info.y_height:info.c_height;
        const auto& current=info.slots[dst].planes[p];
        const auto& previous=info.slots[ref].planes[p];
        const int status=bink_research_decode_video_plane(
            padded.data(),limit,pos,limit,w,h,current.data,current.pitch,
            previous.data,&report.trace);
        const auto cursor=4U*report.trace.input_words;
        report.next_offset=cursor;
        if(status || cursor<=pos || cursor>limit ||
           (p==3 && cursor>layout.alpha_end+4U) ||
           (p==0 && cursor>layout.luma_end+4U) ||
           (p==1 && cursor>=limit))
            return fail(3);
        pos=cursor;
    }
    return 0;
}
}

extern "C" int bink_research_decode_registered_packet(
    const std::uint8_t* packet,std::uint32_t bytes,std::uint32_t flags,
    const t6_bink::RegisteredFrameBuffers* info,
    t6_bink::RegisteredDecodeReport* report);
