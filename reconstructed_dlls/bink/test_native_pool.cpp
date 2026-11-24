// Concurrent sessions on the recovered same-worker Store API.
// No original DLL, real host worker, no sleeps or timing assumptions.
#include "bink_native_api.h"
#include "video_registered.h"
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>
namespace {
using namespace t6_bink;
struct Movie {
    void* h=nullptr;
    RegisteredFrameBuffers info{};
    std::array<std::array<std::vector<std::uint8_t>,4>,2> planes;
    unsigned count=0;
    explicit Movie(std::vector<std::uint8_t>& data) {
        h=BinkOpen(data.data(),0x04104400U);
        assert(h);
        BinkGetFrameBuffersInfo(h,&info);
        assert(info.buffer_count==2);info.frame_num=1;
        count=info.slots[0].planes[3].allocate?4:3;
        for(unsigned s=0;s<2;++s)for(unsigned p=0;p<count;++p) {
            const bool luma=p==0||p==3;
            const auto w=luma?info.y_width:info.c_width,ht=luma?info.y_height:info.c_height;
            auto& v=planes[s][p];v.assign(w*ht,s?0x80:0xB7);
            auto& desc=info.slots[s].planes[p];desc.data=v.data();desc.pitch=w;
        }
        BinkRegisterFrameBuffers(h,&info);
    }
    ~Movie(){BinkClose(h);}
    Movie(const Movie&)=delete;
    Movie& operator=(const Movie&)=delete;
    std::uint32_t field(unsigned offset) const {
        std::uint32_t n=0;std::memcpy(&n,static_cast<const char*>(h)+offset,4);return n;
    }
    void compare(Movie& other) {
        assert(field(0x56c)==other.field(0x56c));
        assert(info.frame_num==other.info.frame_num);
        assert(planes==other.planes);
        const auto n=BinkGetRects(h,0),m=BinkGetRects(other.h,0);
        assert(n==m&&n<=8);
        assert(!std::memcmp(static_cast<const char*>(h)+0x40,
                            static_cast<const char*>(other.h)+0x40,n*16));
    }
};
}
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream input(argv[1],std::ios::binary);assert(input);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    BinkSetSoundTrack(0,nullptr);
    assert(BinkStartAsyncThread(0,nullptr)==1);
    assert(BinkStartAsyncThread(1,nullptr)==1);
    Movie reference(bytes);
    constexpr unsigned N=6;
    std::array<std::unique_ptr<Movie>,N> movies;
    for(auto& m:movies)m=std::make_unique<Movie>(bytes);
    unsigned compared=0;
    for(unsigned i=0;i<16;++i) {
        BinkDoFrame(reference.h);
        // Two numbered worker slots; each receives several independent handles.
        for(unsigned n=0;n<N;++n) {
            const unsigned worker=(i+n)%2;
            assert(BinkDoFrameAsync(movies[n]->h,worker,worker)==1);
            assert(BinkDoFrameAsync(movies[n]->h,worker,worker)==0);
        }
        if(i==7) {
            // Stop enqueues behind in-flight work; joining cannot discard
            // already accepted decode tasks. Waits after join still complete.
            assert(BinkRequestStopAsyncThread(0)==1);
            assert(BinkRequestStopAsyncThread(1)==1);
            assert(BinkWaitStopAsyncThread(0)==1);
            assert(BinkWaitStopAsyncThread(1)==1);
        }
        // Wait in reverse submission order, stressing other-handle completion.
        for(unsigned j=N;j-->0;) {
            assert(BinkDoFrameAsyncWait(movies[j]->h,-1)==1);
            movies[j]->compare(reference);++compared;
        }
        if(i==7) {
            assert(BinkStartAsyncThread(0,nullptr)==1);
            assert(BinkStartAsyncThread(1,nullptr)==1);
        }
        if(i!=15) {
            BinkNextFrame(reference.h);
            for(auto& m:movies)BinkNextFrame(m->h);
        }
    }
    // Close a handle with a pending queued/running job. Its output buffers
    // remain alive until close returns, then the worker slot is reusable.
    {
        Movie pending(bytes);
        assert(BinkDoFrameAsync(pending.h,0,0)==1);
    }
    {
        Movie subsequent(bytes);
        assert(BinkDoFrameAsync(subsequent.h,0,0)==1);
        assert(BinkDoFrameAsyncWait(subsequent.h,-1)==1);
        assert(subsequent.field(0x56c)==1);
    }
    for(unsigned n=0;n<2;++n) {
        assert(BinkRequestStopAsyncThread(n)==1);
        assert(BinkWaitStopAsyncThread(n)==1);
    }
    std::cout<<"Native multi-session worker PASS: "<<compared<<" pixel/rectangle frame comparisons, stop/drain/restart and pending close\n";
}
