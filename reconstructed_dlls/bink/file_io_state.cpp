// Original Bink file I/O callback table installed by 0x21010.
// SHA256 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "file_io_state.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
namespace {
template<class T>T get(const void* h,std::size_t off) {
    T v{};std::memcpy(&v,static_cast<const std::uint8_t*>(h)+off,sizeof(v));return v;
}
template<class T>void put(void* h,std::size_t off,T v) {
    std::memcpy(static_cast<std::uint8_t*>(h)+off,&v,sizeof(v));
}
std::uint32_t u(const void* h,std::size_t off){return get<std::uint32_t>(h,off);}
void inc(void* h,std::size_t off,std::uint32_t v) {put(h,off,u(h,off)+v);}
using Lock=void (*)(void*);
using TryLock=int (*)(void*);
using Timer=std::uint32_t (*)();
using Add=void (*)(void*,std::uint32_t);
using ReadNotify=void (*)(void*,std::uint32_t,std::uint32_t);
using BytesNotify=void (*)(void*,std::uint32_t);
void lock_call(void* io,std::size_t off) {if(auto cb=get<Lock>(io,off))cb(io);}
void add(void* io,std::size_t off,std::uint32_t n) {
    if(auto cb=get<Add>(io,0x78))cb(static_cast<std::uint8_t*>(io)+off,n);
    else inc(io,off,n);
}
std::uint32_t time(void* io) {if(auto cb=get<Timer>(io,0x68))return cb();return 0;}
}
// 0x20990: exact unsigned overflow and conditional move.
extern "C" std::uint32_t bink_research_file_io_reserve(std::uint32_t bytes) {
    return std::max(0x41000U,((bytes+0x1FFFFU)&~0x1FFFFU)+0x1000U);
}
// 0x209B0: align supplied address to 4KiB, truncate capacity to 128KiB.
extern "C" int bink_research_file_set_buffer(void* io,void* memory,std::uint32_t bytes,std::uint32_t end) {
    if(!io || !memory)return -1;
    const auto raw=reinterpret_cast<std::uintptr_t>(memory);
    const auto start=(raw+0xFFFU)&~std::uintptr_t(0xFFF);
    if(start<raw || start-raw>bytes)return -1;
    lock_call(io,0x40);
    const auto n=static_cast<std::uint32_t>(bytes-(start-raw))&~0x1FFFFU;
    put(io,0xD8,start);put(io,0xF0,0U);put(io,0x100,start);
    put(io,0xE0,start+n);put(io,0xA0,n);put(io,0xF8,n);
    put(io,0xAC,0U);put(io,0xEC,end);
    lock_call(io,0x50);
    return 0;
}
// 0x200C0: file-header/index reader.
extern "C" std::int64_t bink_research_file_read_header(void* io,std::uint32_t offset,
    void* dest,std::uint32_t bytes,t6_bink::FileSeek seek,t6_bink::FileRead read,void* ctx) {
    if(!io||!dest||!seek||!read)return -1;
    lock_call(io,0x40);
    if(offset!=UINT32_MAX && offset!=u(io,0xF4)) {
        seek(ctx,get<std::uint64_t>(io,0xC0)+static_cast<std::int32_t>(offset));
        put(io,0xF4,offset);
    }
    const auto got=read(ctx,dest,bytes);
    if(got>bytes) {lock_call(io,0x50);return -1;}
    if(got!=bytes)put(io,0x80,1U);
    inc(io,0xF4,got);
    const auto pos=u(io,0xF4);
    put(io,0xD0,pos);
    put(io,0xA8,std::min(u(io,0xA0),u(io,0xEC)-pos));
    lock_call(io,0x50);
    if(auto cb=get<BytesNotify>(io,0x70))cb(io,got);
    return got;
}
// 0x20AA0: regular sequential buffer refill, without the separate
// end-of-file loop/seek transition. Optional atomic adds are preserved.
extern "C" std::int64_t bink_research_file_refill(void* io,t6_bink::FileRead read,void* ctx) {
    if(!io || !read)return -1;
    if(u(io,0x80)||u(io,0xB0))return 0;
    auto try_lock=get<TryLock>(io,0x48);
    if(!try_lock||!try_lock(io))return -1;
    const auto finish=[&](std::int64_t ret) {
        lock_call(io,0x50);
        return ret;
    };
    const auto pos=u(io,0xF4),end=u(io,0xEC);
    if(pos>=end || u(io,0xF8)<0x20000U || u(io,0x108))return finish(-1);
    const auto start=get<std::uintptr_t>(io,0xD8),limit=get<std::uintptr_t>(io,0xE0);
    const auto cursor=get<std::uintptr_t>(io,0x100);
    if(!start || cursor<start || cursor>=limit || limit-cursor<0x20000U)
        return finish(-1);
    const auto frame_active=u(io,0x8C);
    const auto remaining=end-pos;
    auto count=std::min(remaining,0x20000U);
    const auto before=time(io);
    auto skip=pos&0x1FFFFU;
    if(skip>count || count==remaining)skip=0;
    if(skip)add(io,0xF0,skip);
    count-=skip;
    put(io,0x84,1U);
    const auto got=read(ctx,reinterpret_cast<void*>(cursor+skip),count);
    put(io,0x84,0U);
    if(got>count)return finish(-1);
    if(auto cb=get<ReadNotify>(io,0x60))cb(io,got,skip);
    if(got!=count)put(io,0x80,1U);
    if(got) {
        inc(io,0x88,got);inc(io,0xF4,got);
        const auto next=cursor+0x20000U;
        put(io,0x100,next>=limit?start:next);
        add(io,0xF8,0U-got);add(io,0xAC,got);
        if(u(io,0xAC)>u(io,0xA4))put(io,0xA4,u(io,0xAC));
    }
    const auto elapsed=time(io)-before;
    inc(io,0x90,elapsed);
    inc(io,(frame_active||u(io,0x8C))?0x9C:0x98,elapsed);
    // The original successful-refill path jumps past the A8 snapshot
    // write; that write belongs to the no-refill branch only.
    return finish(got);
}
// 0x20F80: bit 0 has priority; bit 31 invokes the corresponding
// original synchronization/refill boundary even if state did not change.
extern "C" std::uint32_t bink_research_file_control(void* io,std::uint32_t flags,t6_bink::FileFill fill,void* ctx) {
    if(!io)return UINT32_MAX;
    if(flags&1U) {
        if(!u(io,0xB0))put(io,0xB0,1U);
        if(flags&0x80000000U) {lock_call(io,0x40);lock_call(io,0x50);}
    } else if(flags&2U) {
        if(u(io,0xB0)==1)put(io,0xB0,0U);
        if(flags&0x80000000U && fill)fill(io,ctx);
    }
    return u(io,0xB0);
}

extern "C" std::int64_t bink_research_file_read_buffered(void* io,std::uint32_t frame,
    std::uint32_t offset,void* dest,std::uint32_t bytes) {
    (void)frame; // only used by the original telemetry path
    if(!io||!dest)return -1;
    if(u(io,0x80))return 0;
    const auto pos=u(io,0xD0),avail=u(io,0xAC),capacity=u(io,0xA0),cursor=u(io,0xF0);
    const auto* start=get<const std::uint8_t*>(io,0xD8);
    if((offset!=UINT32_MAX && offset!=pos) || bytes>avail || avail>capacity ||
       !capacity || cursor>=capacity || !start || pos>u(io,0xEC) ||
       bytes>u(io,0xEC)-pos)return -1;
    const auto before=time(io);
    auto* output=static_cast<std::uint8_t*>(dest);
    inc(io,0xD0,bytes);
    const auto tail=capacity-cursor;
    if(bytes>=tail) {
        std::memcpy(output,start+cursor,tail);
        output+=tail;
        put(io,0xF0,0U);add(io,0xAC,0U-tail);add(io,0xF8,tail);
        if(bytes>tail) {
            const auto rest=bytes-tail;
            std::memcpy(output,start,rest);
            add(io,0xF0,rest);add(io,0xAC,0U-rest);add(io,0xF8,rest);
        }
    } else if(bytes) {
        std::memcpy(output,start+cursor,bytes);
        add(io,0xF0,bytes);add(io,0xAC,0U-bytes);add(io,0xF8,bytes);
    }
    inc(io,0x94,time(io)-before);
    auto snapshot=std::min(u(io,0xEC)-u(io,0xD0),capacity);
    if(u(io,0xAC)+0x20000U>snapshot)snapshot=u(io,0xAC);
    put(io,0xA8,snapshot);
    if(auto cb=get<BytesNotify>(io,0x70))cb(io,bytes);
    return bytes;
}

// Original frame-read branches at 0x20270 with a platform seek/read boundary.
extern "C" std::int64_t bink_research_file_read_frame(void* io,std::uint32_t frame,
    std::uint32_t offset,void* dest,std::uint32_t bytes,
    t6_bink::FileSeek seek,t6_bink::FileRead read,void* ctx) {
    (void)frame;
    if(!io||!dest||!seek||!read)return -1;
    if(u(io,0x80))return 0;
    const auto capacity=u(io,0xA0);
    const auto start=get<std::uintptr_t>(io,0xD8);
    if(!capacity || !start || u(io,0xF0)>=capacity || u(io,0xAC)>capacity ||
       u(io,0x108))return -1;
    if(offset==UINT32_MAX)offset=u(io,0xD0);
    if(offset>u(io,0xEC)||bytes>u(io,0xEC)-offset)return -1;
    const auto before=time(io);
    bool locked=false;
    const auto acquire=[&] {if(!locked){lock_call(io,0x40);locked=true;}};
    if(offset!=u(io,0xD0)) {
        acquire();
        if(offset>u(io,0xD0)&&offset<=u(io,0xF4)) {
            const auto skip=offset-u(io,0xD0);
            if(skip>u(io,0xAC)) {lock_call(io,0x50);return -1;}
            put(io,0xD0,offset);
            inc(io,0xF8,skip);inc(io,0xAC,0U-skip);inc(io,0xF0,skip);
            if(u(io,0xF0)>=capacity)inc(io,0xF0,0U-capacity);
        } else {
            seek(ctx,get<std::uint64_t>(io,0xC0)+static_cast<std::int32_t>(offset));
            put(io,0xF4,offset);put(io,0xD0,offset);
            put(io,0xF8,capacity);put(io,0xAC,0U);put(io,0xF0,0U);
            put(io,0x100,start);
        }
    }
    auto* output=static_cast<std::uint8_t*>(dest);
    std::uint32_t copied=0;
    while(copied<bytes) {
        const auto available=u(io,0xAC);
        if(available) {
            const auto count=std::min(available,bytes-copied);
            inc(io,0xD0,count);
            const auto cursor=u(io,0xF0),tail=capacity-cursor;
            const auto first=std::min(count,tail);
            std::memcpy(output+copied,reinterpret_cast<const void*>(start+cursor),first);
            if(first==tail)put(io,0xF0,0U);
            else add(io,0xF0,first);
            add(io,0xAC,0U-first);add(io,0xF8,first);
            if(count>first) {
                const auto rest=count-first;
                std::memcpy(output+copied+first,reinterpret_cast<const void*>(start),rest);
                add(io,0xF0,rest);add(io,0xAC,0U-rest);add(io,0xF8,rest);
            }
            copied+=count;
        }
        if(copied==bytes)break;
        if(!locked) {acquire();continue;}
        const auto remaining=bytes-copied;
        const auto start_read=time(io);
        const auto got=read(ctx,output+copied,remaining);
        if(got>remaining) {lock_call(io,0x50);return -1;}
        if(got!=remaining)put(io,0x80,1U);
        if(auto cb=get<ReadNotify>(io,0x60))cb(io,got,0);
        inc(io,0xF4,got);inc(io,0xD0,got);inc(io,0x88,got);
        const auto after=time(io);
        inc(io,0x90,after-start_read);
        inc(io,0x94,after-before);
        copied+=got;
        auto snapshot=std::min(u(io,0xEC)-u(io,0xD0),capacity);
        if(u(io,0xAC)+0x20000U>snapshot)snapshot=u(io,0xAC);
        put(io,0xA8,snapshot);
        lock_call(io,0x50);
        if(auto cb=get<BytesNotify>(io,0x70))cb(io,copied);
        return copied;
    }
    inc(io,0x94,time(io)-before);
    auto snapshot=std::min(u(io,0xEC)-u(io,0xD0),capacity);
    if(u(io,0xAC)+0x20000U>snapshot)snapshot=u(io,0xAC);
    put(io,0xA8,snapshot);
    if(locked)lock_call(io,0x50);
    if(auto cb=get<BytesNotify>(io,0x70))cb(io,copied);
    return copied;
}
