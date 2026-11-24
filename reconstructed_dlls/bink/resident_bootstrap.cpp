// Original Store Bink x64 SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "resident_bootstrap.h"
#include "bink_index.h"
#include <cstring>
#include <limits>
#include <vector>

namespace {
template<class T> void put(void* h,std::size_t off,T value) {
    std::memcpy(static_cast<std::uint8_t*>(h)+off,&value,sizeof(value));
}
bool header(const std::uint8_t* data,std::size_t bytes,t6_bink::HeaderPrefix& h) {
    return data && t6_bink::validate_header_prefix(data,bytes,&h) &&
        h.magic==0x694B4942U && !h.audio_track_count && !(h.flags&~0x00100000U) &&
        h.width>=16 && h.width<=1280 && h.height>=16 && h.height<=720 &&
        h.frame_count && h.frame_count==h.frame_index_count &&
        std::uint64_t(h.payload_bytes)+8==bytes && h.largest_frame_bytes<=262144;
}
}
namespace t6_bink {
bool resident_bootstrap_layout(const HeaderPrefix& h,ResidentBootstrapLayout& out,bool explicit_no_tracks) {
    out={};
    if(!h.rate_numerator || !h.rate_denominator || h.audio_track_count)return false;
    // Original 32-bit unsigned add/div; cap the research allocation
    // for malformed or unsupported high-frame-rate files.
    auto n=(h.rate_numerator+(h.rate_denominator>>1))/h.rate_denominator;
    if(!n)n=1;
    if(n>1000)return false;
    out.history_count=n;
    std::uint32_t total=0;
    // Every region starts at the previous cumulative offset. Original
    // 0x17F69 rounds size to 32, then pads using the current cache-line
    // index and the rounded size's line count modulo 32.
    auto reserve=[&](std::uint32_t bytes) {
        const auto offset=0x5C0U+total;
        const auto rounded=(bytes+31U)&~31U;
        const auto pad=(((total>>5)+1-(rounded>>5))&31U)<<5;
        total+=rounded+pad;
        return offset;
    };
    for(auto& off:out.history_offsets)off=reserve(4*n);
    out.dummy_audio_offset=reserve(0x1C0);
    out.track_ids_offset=reserve(0);
    // With BinkSetSoundTrack(0,null) and flag 0x4000, the original
    // omits the final region entirely (track-ID pointer is one-past-end).
    if(!explicit_no_tracks)out.init_region_offset=reserve(0x104);
    out.allocation_bytes=0x5C0U+total;
    out.accounted_bytes=0x5B8U+total;
    return true;
}
bool init_resident_external(void* handle,std::size_t capacity,
    const std::uint8_t* data,std::size_t bytes,std::uint32_t flags,
    ResidentTimer timer,void* ctx) {
    HeaderPrefix h{};
    ResidentBootstrapLayout plan{};
    if(!handle || !timer || !header(data,bytes,h) ||
       (reinterpret_cast<std::uintptr_t>(handle)&7U) ||
       (reinterpret_cast<std::uintptr_t>(data)&3U) ||
       (flags&~0x00104000U)!=0x04000400U ||
       !resident_bootstrap_layout(h,plan,(flags&0x4000U)!=0) || capacity<plan.allocation_bytes)
        return false;
    std::vector<FrameSpan> spans(h.frame_count);
    if(!read_video_only_index(data,bytes,spans.data(),spans.size(),&h) ||
       !spans.front().keyframe || spans.back().end!=bytes)return false;
    // 0x185** copies the 0x5B8 zero-initialized temporary handle into
    // the aligned allocation, then points at the original indexed input.
    // These fields are directly observed in the real memory-open oracle.
    std::memset(handle,0,plan.allocation_bytes);
    auto* state=static_cast<std::uint8_t*>(handle);
    const auto* index=reinterpret_cast<const std::uint32_t*>(data+44);
    const auto flag_word=(flags&~(h.flags&0x00100000U?0U:0x00100000U))|0x08000000U;
    put(state,0x00,h.width);put(state,0x04,h.height);put(state,0x08,h.frame_count);
    put(state,0x14,h.rate_numerator);put(state,0x18,h.rate_denominator);
    put(state,0x20,flag_word);put(state,0x24,h.flags);put(state,0x28,h.payload_bytes);
    const auto block_columns=(((h.width+15U)>>4)+15U)&~15U;
    put(state,0x110,block_columns);put(state,0x114,block_columns*((h.height+15U)>>4));
    put(state,0x118,h.largest_frame_bytes);put(state,0x11C,h.frame_index_count);
    put(state,0x134,1U);
    put(state,0x1B8,h.width);put(state,0x1BC,h.height);
    put(state,0x108,state+plan.track_ids_offset);
    put(state,0x198,state+plan.dummy_audio_offset);
    put(state,0x1A0,index);put(state,0x1C0,index);put(state,0x1C8,index);put(state,0x1D0,index);
    put(state,0x1D8,data+spans.front().start);
    put(state,0x228,state);
    constexpr std::size_t rings[]={0x4D8,0x4E0,0x4E8,0x4F0,0x4F8,0x500,0x508};
    for(unsigned i=0;i<7;++i)put(state,rings[i],state+plan.history_offsets[i]);
    put(state,0x510,plan.history_count);put(state,0x528,plan.accounted_bytes);
    put(state,0x560,h.rate_numerator);put(state,0x564,h.rate_denominator);
    put(state,0x59C,h.magic);
    // Two earlier original clock reads occur before select-frame's
    // initial-frame clock read. Unknown timestamp uses are not claimed.
    (void)timer(ctx);(void)timer(ctx);
    if(bink_research_select_resident_frame(state,1,timer,ctx))return false;
    put(state,0x10,UINT32_MAX);
    return true;
}
}
extern "C" std::uint32_t bink_research_resident_bootstrap_size(const std::uint8_t* data,std::size_t bytes,std::uint32_t flags) {
    t6_bink::HeaderPrefix h{};
    t6_bink::ResidentBootstrapLayout plan{};
    return header(data,bytes,h)&&t6_bink::resident_bootstrap_layout(h,plan,(flags&0x4000U)!=0)?plan.allocation_bytes:0;
}
extern "C" int bink_research_init_resident_external(void* handle,std::size_t capacity,
    const std::uint8_t* data,std::size_t bytes,std::uint32_t flags,t6_bink::ResidentTimer timer,void* ctx) {
    return t6_bink::init_resident_external(handle,capacity,data,bytes,flags,timer,ctx)?0:1;
}
