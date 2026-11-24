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
    const auto first=read<std::uint32_t>(table,0)&~1U;
    const auto raw=read<std::uint32_t>(table,std::size_t(i)*4);
    const auto offset=raw&~1U;
    // Preserve original 32-bit arithmetic and key bit.
    put(h,0x1EC,i+1);
    put(h,0x1E8,offset);
    put(h,0x140,raw&1U);
    put(h,0x13C,(read<std::uint32_t>(table,std::size_t(i+1)*4)&~1U)-offset);
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

extern "C" std::int64_t bink_research_pause_video_only(void* h,std::uint32_t paused,
    t6_bink::ResidentTimer timer,void* ctx) {
    if(!h)return 0;
    if(!timer || u32(h,0x144))return -1;
    const auto now=timer(ctx);
    const auto stamp=u32(h,0x530);
    if(stamp && stamp!=UINT32_MAX) {
        put(h,0x530,0);
        put(h,0x558,u32(h,0x558)+now-stamp);
    }
    const auto old=u32(h,0x128);
    put(h,0x570,0);
    put(h,0x128,paused);
    const auto after=timer(ctx);
    // Preserve original zero/nonzero transitions and raw pause word.
    if(!paused && old)
        put(h,0x534,after-u32(h,0x58C));
    else if(paused && !old) {
        const auto start=u32(h,0x534);
        put(h,0x58C,start?after-start:0);
    }
    return u32(h,0x128);
}

extern "C" int bink_research_prepare_video_frame(void* h,std::uint32_t now) {
    if(!h || u32(h,0x0C)==u32(h,0x56C))return 0;
    if(u32(h,0x144))return -1;
    if(u32(h,0x270))put(h,0x1C,1);
    if(u32(h,0x1C))return 0;
    // Original 0x1A2D4/0x1A2E1 call audio toggle if resync + sound-on.
    // Do not claim that unimplemented path; reject before further writes.
    if(u32(h,0x578) && u32(h,0x130))return -1;
    const auto stamp=u32(h,0x530);
    if(stamp && stamp!=UINT32_MAX) {
        put(h,0x530,0);
        put(h,0x558,u32(h,0x558)+now-stamp);
    }
    if(u32(h,0x578)) {
        put(h,0x578,0);
        put(h,0x534,0);
        put(h,0x58C,0);
    }
    if(u32(h,0x128)) {
        put(h,0x534,0);
        put(h,0x58C,0);
    }
    put(h,0x57C,0);
    put(h,0x27C,1);
    auto* dirty=read<void*>(h,0xE0);
    if(u32(h,0x134) && read<void*>(h,0xD0) &&
       u32(h,0x59C)!=0x6132424BU && !u32(h,0x12C) && dirty)
        std::memset(dirty,0,u32(h,0x114)); // original 0x5E10
    put(h,0x148,0);
    write(h,0xE8,dirty);
    return 1;
}

extern "C" int bink_research_finish_video_frame(void* h,std::uint32_t now,
    t6_bink::ResidentTimer timer,void* ctx) {
    if(!h || !timer || u32(h,0x144))return -1;
    const auto n=u32(h,0x510),old_index=u32(h,0x514);
    if(!n || old_index>=n || n>0x7fffffffU ||
       (!u32(h,0x524) && !u32(h,0x560)))return -1;
    constexpr std::size_t ring_offsets[]={0x4D8,0x4E8,0x4E0,0x4F0,0x4F8,0x500,0x508};
    for(auto off:ring_offsets)if(!read<std::uint32_t*>(h,off))return -1;
    // Original swaps the registered slot only if the other Y pointer exists.
    auto* desc=read<void*>(h,0xD0);
    if(desc) {
        const auto slot=u32(desc,0x14);
        if(slot>1)return -1;
        const auto next=slot^1U;
        if(read<void*>(desc,0x20+next*0x60))put(desc,0x14,next);
    }
    const auto index=old_index?old_index-1:n-1;
    put(h,0x514,index);
    const auto previous=u32(h,0x570);
    if(previous) {
        const auto elapsed=now-previous;
        const auto worst=u32(h,0x540);
        if(elapsed>worst) {
            put(h,0x548,worst);
            put(h,0x54C,u32(h,0x544));
            put(h,0x540,elapsed);
            put(h,0x544,u32(h,0x56C));
        // The original compares the absolute tick (EDX), not elapsed (ECX).
        } else if(now>u32(h,0x548)) {
            put(h,0x548,elapsed);
            put(h,0x54C,u32(h,0x56C));
        }
    }
    put(h,0x570,now);
    const std::uint32_t values[]={now,u32(h,0x550),u32(h,0x554),u32(h,0x558),
        u32(h,0x284),u32(h,0x288),u32(h,0x28C)};
    for(unsigned i=0;i<7;++i)
        read<std::uint32_t*>(h,ring_offsets[i])[index]=values[i];
    if(!u32(h,0x524)) {
        const auto ms=static_cast<std::uint32_t>(std::uint64_t(u32(h,0x564))*1000/u32(h,0x560));
        read<std::uint32_t*>(h,0x4D8)[old_index]=now-ms;
        put(h,0x524,now);
        put(h,0x28C,0);
        put(h,0x288,0);
    }
    const auto dirty=read<void*>(h,0xE8);
    write(h,0xF0,dirty);
    if(dirty)put(h,0x3C,UINT32_MAX);
    put(h,0x520,u32(h,0x520)+1);
    if(!u32(h,0x534)) {
        const auto tick=timer(ctx);
        put(h,0x58C,0);
        put(h,0x534,tick);
        put(h,0x538,u32(h,0x520)-1);
    }
    put(h,0x530,UINT32_MAX);
    put(h,0x56C,u32(h,0x0C));
    put(h,0x10,u32(h,0x0C));
    put(h,0x34,u32(h,0x148));
    put(h,0x27C,0);
    return 0;
}

extern "C" int bink_research_get_realtime(void* h,t6_bink::RealtimeSnapshot* out,
    std::uint32_t frames,t6_bink::ResidentTimer timer,void* ctx) {
    if(!h || !out)return 0;
    if(!timer)return -1;
    // 0x1C290: expire the pending timing stamp before computing history.
    const auto now=timer(ctx);
    const auto stamp=u32(h,0x530);
    if(stamp && stamp!=UINT32_MAX) {
        put(h,0x530,0);
        put(h,0x558,u32(h,0x558)+now-stamp);
    }
    const auto n=u32(h,0x510);
    const auto index=u32(h,0x514);
    const auto frame=u32(h,0x0C);
    const auto* table=read<const std::uint32_t*>(h,0x1A0);
    if(n<2 || index>=n || !frame || frame>u32(h,0x08) || !table)return -1;
    if(!frames || frames>=n)frames=n-1;
    if(frame<frames)frames=frame-1;
    const auto divisor=std::uint64_t(u32(h,0x564))*frames;
    if(!divisor)return -1;
    constexpr std::size_t ring_offsets[]={0x4D8,0x4E8,0x4E0,0x4F8,0x500,0x508,0x4F0};
    for(auto off:ring_offsets)if(!read<const std::uint32_t*>(h,off))return -1;
    auto older=index+frames;
    if(older>=n)older-=n;
    // The original subtracts the raw index entries (including key bits),
    // widens both entries before 64-bit subtraction and multiplication.
    // Original 0x1C33*: REX.W sub/imul, unsigned div, store EAX.
    const auto product=(std::uint64_t(read<std::uint32_t>(table,std::size_t(frame)*4))-
        read<std::uint32_t>(table,std::size_t(frame-frames)*4))*u32(h,0x560);
    t6_bink::RealtimeSnapshot result{};
    result.last_frame=u32(h,0x10);
    result.rate_numerator=u32(h,0x14);
    result.rate_denominator=u32(h,0x18);
    result.sample_frames=frames;
    result.state_298=u32(h,0x298);
    result.state_29c=u32(h,0x29C);
    result.byte_rate=static_cast<std::uint32_t>(std::uint64_t(product)/divisor);
    std::uint32_t delta[7]{};
    for(unsigned i=0;i<7;++i) {
        const auto* ring=read<const std::uint32_t*>(h,ring_offsets[i]);
        delta[i]=ring[index]-ring[older];
    }
    result.elapsed=delta[0];result.history_4e8=delta[1];result.history_4e0=delta[2];
    result.history_4f8=delta[3];result.history_500=delta[4];result.history_508=delta[5];
    result.history_4f0=delta[6];
    *out=result;
    return 0;
}

namespace {
struct DirtyRect {std::int32_t x,y,w,h;};
// 0x1C440: trim empty outside rows/columns, 16x16 dirty-map cells.
// The output is left untouched if no cell in the supplied rectangle is set.
bool bound_dirty(DirtyRect& out,const std::uint8_t* map,std::uint32_t stride,DirtyRect in) {
    int x0=in.x/16,y0=in.y/16,x1=x0+in.w/16,y1=y0+in.h/16;
    int left=x1,top=y1,right=x0-1,bottom=y0-1;
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)if(map[std::size_t(y)*stride+x]) {
        if(x<left)left=x;
        if(x>right)right=x;
        if(y<top)top=y;
        if(y>bottom)bottom=y;
    }
    if(right<left)return false;
    out={left*16,top*16,(right-left+1)*16,(bottom-top+1)*16};return true;
}
struct DirtySplit {DirtyRect first{},second{};int saving=0;};
// 0x1C610: split at half width/height rounded up to 16. The
// horizontal (top/bottom) candidate replaces the vertical one only
// if strictly better. Tight input guarantees nonempty halves.
DirtySplit split_dirty(DirtyRect r,const std::uint8_t* map,std::uint32_t stride) {
    DirtySplit best{};
    const auto consider=[&](DirtyRect a,DirtyRect b) {
        DirtyRect aa{},bb{};
        if(!bound_dirty(aa,map,stride,a)||!bound_dirty(bb,map,stride,b))return;
        const int saving=r.w*r.h-aa.w*aa.h-bb.w*bb.h;
        if(saving>best.saving)best={aa,bb,saving};
    };
    if(r.w>=32) {
        const int n=(r.w/2+15)&~15;
        consider({r.x,r.y,n,r.h},{r.x+n,r.y,r.w-n,r.h});
    }
    if(r.h>=32) {
        const int n=(r.h/2+15)&~15;
        consider({r.x,r.y,r.w,n},{r.x,r.y+n,r.w,r.h-n});
    }
    return best;
}
int scan_dirty_rects(void* h,const std::uint8_t* dirty,std::uint32_t surface) {
    const auto width=u32(h,0),height=u32(h,4),stride=u32(h,0x110),length=u32(h,0x114);
    if(!width||!height||width>32767||height>32767)return -1;
    const auto cols=(width+15)/16,rows=(height+15)/16;
    if(stride<cols || std::uint64_t(stride)*rows>length)return -1;
    DirtyRect r[8]{};
    if(!bound_dirty(r[0],dirty,stride,{0,0,int(cols*16),int(rows*16)})) {
        put(h,0x3C,0);return 0;
    }
    unsigned count=1;
    // Bit 27 jumps around the copy of the trimmed rectangle into the
    // stack list; it keeps the original full rounded rectangle if any
    // dirty cell exists. The all-clean path still returns zero.
    if(surface&0x08000000U)r[0]={0,0,int(cols*16),int(rows*16)};
    else {
        while(count<8) {
            DirtySplit best{};unsigned index=0;
            for(unsigned i=0;i<count;++i) {
                auto s=split_dirty(r[i],dirty,stride);
                if(s.saving>best.saving){best=s;index=i;}
            }
            if(!best.saving)break;
            r[index]=best.first;r[count++]=best.second;
        }
    }
    put(h,0x3C,count);
    // Original orders by y<<16 plus x before applying scale/clipping.
    for(unsigned i=0;i<count;++i) {
        unsigned j=0;
        for(unsigned k=1;k<count;++k)
            if(r[k].y<r[j].y || (r[k].y==r[j].y && r[k].x<r[j].x))j=k;
        auto a=r[j];r[j].y=0x7fffffff;
        const auto scale=u32(h,0x568)&0x70000000U;
        if(scale==0x30000000U||scale==0x40000000U||scale==0x50000000U){a.x*=2;a.w*=2;}
        if(scale==0x10000000U||scale==0x20000000U||scale==0x40000000U||scale==0x50000000U){a.y*=2;a.h*=2;}
        if(std::uint32_t(a.x+a.w)>width)a.w=int(width)-a.x;
        if(std::uint32_t(a.y+a.h)>height)a.h=int(height)-a.y;
        write(h,0x40+i*16,a);
    }
    return static_cast<int>(count);
}
}

extern "C" std::int64_t bink_research_get_rects(void* h,std::uint32_t surface_type,
    t6_bink::RectAllocate allocate,void* ctx) {
    // Original branches preceding both allocation and rectangle scanning.
    // 0x1C837 tests the argument, not handle flags. Bit 19 on either
    // argument or handle bypasses the 0x57C == 2 suppression.
    if(!h || (surface_type&0x04000000U))return 0;
    if(!((surface_type|u32(h,0x20))&0x00080000U) && u32(h,0x57C)==2)return 0;
    const auto* dirty=read<void*>(h,0xF0);
    const auto cached=u32(h,0x3C);
    if(!read<void*>(h,0xD8)) {
        const auto length=u32(h,0x114);
        if(!allocate || length>UINT32_MAX-47U)return -1;
        // 0x1C86D: both 100 values, allocation accounting before align32.
        put(h,0x148,100);
        put(h,0x34,100);
        const auto requested=length+16;
        put(h,0x528,u32(h,0x528)+requested);
        auto* buffer=static_cast<std::uint8_t*>(allocate((requested+31U)&~31U,ctx));
        if(!buffer)return -1; // original would fault on null allocation
        write(h,0xD8,buffer);
        write(h,0xE0,buffer);
        buffer[length]=0;
    }
    if(!dirty) {
        put(h,0x3C,1);
        put(h,0x40,0);put(h,0x44,0);
        put(h,0x48,u32(h,0x00));put(h,0x4C,u32(h,0x04));
        return 1;
    }
    if(cached==UINT32_MAX)return scan_dirty_rects(h,static_cast<const std::uint8_t*>(dirty),surface_type);
    return cached;
}

extern "C" std::uint32_t bink_research_control_background_io(void* h,std::uint32_t control) {
    if(!h)return UINT32_MAX;
    const auto callback=read<t6_bink::BackgroundIO>(h,0x220);
    if(!callback)return UINT32_MAX;
    return callback(static_cast<std::uint8_t*>(h)+0x1F0,control);
}

extern "C" int bink_research_submit_async(void* h,std::uint32_t first,
    std::uint32_t second,t6_bink::ResidentTimer timer,void* clock_ctx,
    t6_bink::RectAllocate allocate,void* alloc_ctx,
    t6_bink::AsyncEnqueue enqueue,void* queue_ctx,char* error,
    std::uintptr_t* worker_entry,std::uintptr_t entry_address) {
    if(!h)return 0;
    if(!error || !worker_entry)return -1;
    if(u32(h,0x120)||u32(h,0x124)) {
        constexpr char msg[]="There is already an async decompression in progress on this HBINK.";
        std::memcpy(error,msg,sizeof(msg));
        return 0;
    }
    if(!timer || !enqueue || u32(h,0x144) ||
       (reinterpret_cast<std::uintptr_t>(h)&7U))return -1;
    const auto* first_map=read<void*>(h,0xD8);
    const auto* active_map=read<void*>(h,0xE0);
    if(first_map && (!active_map || first_map==active_map)) {
        const auto length=u32(h,0x114);
        if(!allocate || length>UINT32_MAX-47U)return -1;
        const auto requested=length+16;
        put(h,0x528,u32(h,0x528)+requested);
        auto* buffer=static_cast<std::uint8_t*>(allocate((requested+31U)&~31U,alloc_ctx));
        if(!buffer)return -1;
        write(h,0xE0,buffer);
        buffer[length]=0;
    }
    const auto prepared=bink_research_prepare_video_frame(h,timer(clock_ctx));
    if(prepared!=1)return prepared;
    // 0x21494 writes the worker entry even if queueing subsequently fails.
    *worker_entry=entry_address;
    const auto address=reinterpret_cast<std::uintptr_t>(h);
    if(enqueue(first,address|(first==second?6U:2U),queue_ctx))
        put(h,0x120,first|0x300U);
    // Original optionally copies the worker-queue error pointer into g_error.
    // This narrow dispatcher assumes that pointer is null; queue-error
    // publication is a separate boundary, not yet implemented.
    if(first!=second && enqueue(second,address|5U,queue_ctx))
        put(h,0x124,second|0x300U);
    return (u32(h,0x120)||u32(h,0x124))?1:0;
}

extern "C" int bink_research_async_wait_idle(const void* h) {
    return !h || (!u32(h,0x120) && !u32(h,0x124)) ? 1 : -1;
}

extern "C" int bink_research_wait_async(void* h,std::int32_t timeout,
    std::uint32_t active_mask,std::uint32_t running_mask,
    t6_bink::ResidentTimer timer,void* clock_ctx,
    t6_bink::AsyncReceive receive,t6_bink::AsyncNotify notify,
    void* queue_ctx,char* error) {
    if(!h || (!u32(h,0x120)&&!u32(h,0x124)))return 1;
    if(!timer || !receive || !notify || !error || u32(h,0x144))return -1;
    const auto start=timeout>0?timer(clock_ctx):0;
    const auto publish=[&](const char* message) {
        if(!message)return;
        // 0x216F3: copy a NUL-terminated error, limited to 255 bytes.
        std::size_t i=0;
        for(;i<254 && message[i];++i)error[i]=message[i];
        error[i]=0;
    };
    for(;;) {
        for(unsigned slot=0;slot<2;++slot) {
            const auto off=0x120U+slot*4;
            if(!(u32(h,off)&0x100U))continue;
            auto delay=timeout;
            for(;;) {
                const auto worker=u32(h,off)&~0x300U;
                // Original uses x86's modulo-32 shift, then tests the
                // active mask before the range and running-mask checks.
                const auto bit=1U<<(worker&31U);
                if(!(active_mask&bit)) {
                    if(worker>=8) {publish("Out of range thread number.");break;}
                    if(!(running_mask&bit)) {publish("Invalid thread number.");break;}
                }
                if(worker>=8)return -1; // unsafe original out-of-array access
                t6_bink::AsyncCompletion completion{};
                const char* queue_error=nullptr;
                if(!receive(worker,delay,&completion,&queue_error,queue_ctx)) {
                    publish(queue_error);
                    break;
                }
                auto* owner=reinterpret_cast<void*>(completion.tagged_handle&~std::uintptr_t(1));
                if(!owner || (reinterpret_cast<std::uintptr_t>(owner)&7U))return -1;
                const auto target=0x120U+4U*(completion.tagged_handle&1U);
                if(owner!=h)notify(u32(owner,target)&~0x300U,queue_ctx);
                put(owner,target,u32(owner,target)&~0x100U);
                delay=0;
            }
        }
        if(!((u32(h,0x120)|u32(h,0x124))&0x100U)) {
            put(h,0x120,0);put(h,0x124,0);
            // 0x21791: swap the two dirty maps, then invoke the
            // same original finish helper used by the synchronous path.
            auto* a=read<void*>(h,0xD8);
            auto* b=read<void*>(h,0xE0);
            if(a && b) {write(h,0xD8,b);write(h,0xE0,a);}
            return bink_research_finish_video_frame(h,timer(clock_ctx),timer,clock_ctx)==0?1:-1;
        }
        if(!timeout)return 0;
        if(timeout>0 && timer(clock_ctx)-start>=static_cast<std::uint32_t>(timeout))return 0;
    }
}

extern "C" int bink_research_select_stream_frame(void* h,std::uint32_t frame,
    t6_bink::ResidentTimer timer,void* ctx) {
    if(!h||!timer||!u32(h,0x08)||frame>u32(h,0x08)||u32(h,0x144)||
       u32(h,0x138)||read<void*>(h,0x1D8)||!read<void*>(h,0x5A0)||
       !read<void*>(h,0x1A0))return static_cast<int>(Result::unsupported);
    const auto i=frame?frame-1:0;
    put(h,0x57C,0);put(h,0x10,u32(h,0x0C));
    if(i==0) {
        const auto now=timer(ctx);
        if(!u32(h,0x574))put(h,0x574,now);
        if(now-u32(h,0x574)>300000U){put(h,0x574,now);put(h,0x578,1);}
    }
    const auto* table=read<const std::uint32_t*>(h,0x1A0);
    const auto raw=read<std::uint32_t>(table,std::size_t(i)*4);
    const auto offset=raw&~1U;
    const auto next=read<std::uint32_t>(table,std::size_t(i+1)*4)&~1U;
    put(h,0x1EC,i+1);put(h,0x1E8,offset);put(h,0x140,raw&1U);
    put(h,0x13C,next-offset);
    const auto scratch=read<std::uintptr_t>(h,0x5A0);
    const auto aligned=(scratch+0x100U)&~std::uintptr_t(0x7F);
    write(h,0x100,aligned-(0x80U-(offset&0x7FU)));
    put(h,0x138,1U);put(h,0x0C,i+1);
    return 0;
}
extern "C" int bink_research_next_stream_frame(void* h,t6_bink::ResidentTimer timer,void* ctx) {
    if(!h)return 0;
    if(u32(h,0x120)||u32(h,0x124))return static_cast<int>(Result::busy);
    if(!timer||!u32(h,0x08)||u32(h,0x144)||u32(h,0x138))
        return static_cast<int>(Result::unsupported);
    put(h,0x27C,1);
    const auto now=timer(ctx),paused=u32(h,0x530);
    if(paused && paused!=UINT32_MAX) {
        put(h,0x530,0);put(h,0x558,u32(h,0x558)+now-paused);
    }
    const auto current=u32(h,0x0C);
    const auto result=bink_research_select_stream_frame(h,current<u32(h,0x08)?current+1:1,timer,ctx);
    put(h,0x27C,0);
    return result;
}
