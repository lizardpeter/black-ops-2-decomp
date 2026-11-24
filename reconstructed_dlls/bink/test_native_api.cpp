#include "bink_native_api.h"
#include "video_registered.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace {
std::unordered_set<void*> blocks;
unsigned alloc_calls=0,free_calls=0;
void* alloc(std::size_t n) {
    auto* p=std::malloc(n);assert(p);assert(blocks.insert(p).second);++alloc_calls;return p;
}
void release(void* p) {
    assert(blocks.erase(p)==1);++free_calls;std::free(p);
}
std::uint32_t read32(void* h,std::size_t offset) {
    std::uint32_t n;std::memcpy(&n,static_cast<std::uint8_t*>(h)+offset,4);return n;
}
}
int main(int argc,char** argv) {
    using namespace t6_bink;
    assert(argc>=3);
    bool async=false,file=false,stream=false;
    for(int i=3;i<argc;++i) {
        const std::string_view a(argv[i]);
        if(a=="--async")async=true;
        else if(a=="--file")file=true;
        else if(a=="--stream")file=stream=true;
        else assert(false);
    }
    std::vector<std::uint8_t> bytes;
    if(!file) {
        std::ifstream input(argv[1],std::ios::binary);assert(input);
        bytes.assign(std::istreambuf_iterator<char>(input),{});
    }
    assert(BinkStartAsyncThread(8,nullptr)==0);
    assert(BinkRequestStopAsyncThread(7)==0);
    assert(BinkWaitStopAsyncThread(7)==0);
    if(async) {
        assert(BinkStartAsyncThread(0,nullptr)==1);
        assert(BinkStartAsyncThread(0,nullptr)==0);
        assert(BinkStartAsyncThread(1,nullptr)==1);
    }
    BinkSetMemory(alloc,release);
    BinkSetSoundTrack(0,nullptr);
    if(file)BinkSetIOSize(stream?0x10000U:0x1000000U);
    auto* h=BinkOpen(file?static_cast<void*>(argv[1]):static_cast<void*>(bytes.data()),
                     file?0x01104400U:0x04104400U);
    if(!h){std::cerr<<BinkGetError()<<"\n";return 1;}
    assert(reinterpret_cast<std::uintptr_t>(h)%32==0);
    assert(alloc_calls==(file?2U:1U));
    // Closing must use the saved per-allocation callback, not the new global.
    BinkSetMemory(nullptr,nullptr);
    RegisteredFrameBuffers info{};
    BinkGetFrameBuffersInfo(h,&info);
    assert(info.buffer_count==2);info.frame_num=1;
    const unsigned planes=info.slots[0].planes[3].allocate?4:3;
    std::array<std::array<std::vector<std::uint8_t>,4>,2> storage;
    for(unsigned s=0;s<2;++s)for(unsigned p=0;p<planes;++p) {
        const bool luma=p==0||p==3;
        const auto w=luma?info.y_width:info.c_width,ht=luma?info.y_height:info.c_height;
        auto& v=storage[s][p];v.assign(w*ht,s?0x80:0xB7);
        info.slots[s].planes[p].data=v.data();info.slots[s].planes[p].pitch=w;
    }
    BinkRegisterFrameBuffers(h,&info);
    if(file){assert(BinkControlBackgroundIO(h,2)==0);assert(BinkControlBackgroundIO(h,1)==1);}
    else assert(BinkControlBackgroundIO(h,1)==UINT32_MAX);
    std::ofstream out(argv[2],std::ios::binary);assert(out);
    std::ofstream rectout(std::string(argv[2])+".rects",std::ios::binary);assert(rectout);
    const auto frames=stream?read32(h,8):16U;
    for(unsigned i=0;i<frames;++i) {
        assert(read32(h,0xC)==i+1);
        if(i==4){assert(BinkPause(h,1)==1);assert(BinkPause(h,0)==0);}
        if(async) {
            assert(BinkDoFrameAsyncWait(h,0)==1);
            assert(BinkDoFrameAsync(h,i%2,i%2)==1);
            assert(BinkDoFrameAsyncWait(h,-1)==1);
        } else BinkDoFrame(h);
        if(read32(h,0x56C)!=i+1){std::cerr<<BinkGetError()<<"\n";return 2;}
        assert(info.frame_num==i%2);
        for(unsigned p=0;p<planes;++p) {
            auto& v=storage[info.frame_num][p];
            out.write(reinterpret_cast<const char*>(v.data()),v.size());
        }
        const auto n=BinkGetRects(h,0);
        assert(n<=8 && n==read32(h,0x3c));
        rectout.write(reinterpret_cast<const char*>(&n),4);
        rectout.write(static_cast<const char*>(h)+0x40,n*16);
        assert(BinkGetRects(h,0)==n); // cached path
        if(i+1!=frames)BinkNextFrame(h);
    }
    RealtimeSnapshot rt{};BinkGetRealtime(h,&rt,2);
    out.close();assert(out);
    BinkClose(h);assert(blocks.empty()&&alloc_calls==free_calls);
    if(async) {
        for(unsigned n=0;n<2;++n) {
            assert(BinkRequestStopAsyncThread(n)==1);
            assert(BinkWaitStopAsyncThread(n)==1);
            assert(BinkStartAsyncThread(n,nullptr)==1);
            assert(BinkRequestStopAsyncThread(n)==1);
            assert(BinkWaitStopAsyncThread(n)==1);
        }
    }
    std::cout<<"Static native API PASS: "<<frames<<" frames, "<<alloc_calls<<" original-layout allocations freed\n";
}
