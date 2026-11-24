// Original Store Bink x64 SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "resident_frame_state.h"
#include <cstddef>
#include <cstring>

namespace {
using Result=t6_bink::ResidentStateResult;
template<typename T> T read(const void* h,std::size_t off) {
    T v{};
    std::memcpy(&v,static_cast<const std::uint8_t*>(h)+off,sizeof(v));
    return v;
}
template<typename T> void write(void* h,std::size_t off,T v) {
    std::memcpy(static_cast<std::uint8_t*>(h)+off,&v,sizeof(v));
}
std::uint32_t u32(const void* h,std::size_t off) {
    return read<std::uint32_t>(h,off);
}
void put(void* h,std::size_t off,std::uint32_t v) {
    write(h,off,v);
}
bool supported(const void* h,t6_bink::ResidentTimer timer) {
    return h && timer && u32(h,0x08)>0 && u32(h,0x144)==0 &&
        u32(h,0x138)==0 && read<std::uintptr_t>(h,0x1D8)!=0 &&
        read<const std::uint32_t*>(h,0x1A0)!=nullptr;
}
}
extern "C" int bink_research_select_resident_frame(void* h,std::uint32_t frame,
    t6_bink::ResidentTimer timer,void* ctx) {
    if(!supported(h,timer) || frame>u32(h,0x08))
        return static_cast<int>(Result::unsupported);
    // 0x16440: convert public 1-based frame to index; zero also selects 1.
    const std::uint32_t i=frame?frame-1:0;
    put(h,0x57C,0);
    // No audio callback, no outstanding I/O; original 0x162D0 returns.
    put(h,0x10,u32(h,0x0C));
    if(i==0) {
        const auto now=timer(ctx);
        if(u32(h,0x574)==0)put(h,0x574,now);
        if(now-u32(h,0x574)>300000U) {
            put(h,0x574,now);
            put(h,0x578,1);
        }
    }
    const auto* table=read<const std::uint32_t*>(h,0x1A0);
    const auto first=table[0]&~1U;
    const auto raw=table[i];
    const auto offset=raw&~1U;
    // Preserve original 32-bit arithmetic and key bit.
    put(h,0x1EC,i+1);
    put(h,0x1E8,offset);
    put(h,0x140,raw&1U);
    put(h,0x13C,(table[i+1]&~1U)-offset);
    write(h,0x100,read<std::uintptr_t>(h,0x1D8)+std::uint32_t(offset-first));
    // Resident branch skips the streaming refill and telemetry (disabled).
    put(h,0x0C,i+1);
    return static_cast<int>(Result::ok);
}

extern "C" int bink_research_next_resident_frame(void* h,
    t6_bink::ResidentTimer timer,void* ctx) {
    // 0x1B120: null and pending async work return without touching state.
    if(!h)return static_cast<int>(Result::ok);
    if(u32(h,0x120)||u32(h,0x124))return static_cast<int>(Result::busy);
    if(!supported(h,timer))return static_cast<int>(Result::unsupported);
    // 0x1B234: audio-free branch, skip all audio-specific resync.
    put(h,0x27C,1);
    const auto now=timer(ctx);
    const auto paused=u32(h,0x530);
    if(paused!=0 && paused!=UINT32_MAX) {
        put(h,0x530,0);
        put(h,0x558,u32(h,0x558)+now-paused);
    }
    const auto current=u32(h,0x0C);
    const auto frame=current<u32(h,0x08)?current+1:1;
    const auto result=bink_research_select_resident_frame(h,frame,timer,ctx);
    put(h,0x27C,0);
    return result;
}
