// Native integrated session: same original-derived frame state routines,
// real compressed packets, external buffers, and independent slot swaps.
// Outputs only padded planes for comparison with the original coordinator.
#include "resident_video_session.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>

int main(int argc,char** argv) {
    using namespace t6_bink;
    assert(argc==3);
    std::ifstream input(argv[1],std::ios::binary);
    assert(input);
    std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(input)),{});
    std::uint32_t tick=1000;
    const auto clock=[](void* p)->std::uint32_t {
        auto& t=*static_cast<std::uint32_t*>(p);return ++t;
    };
    ResidentVideoSession movie;
    assert(!movie.open({},clock,&tick));
    assert(movie.open(std::move(file),clock,&tick));
    assert(movie.frame_number()==1 && movie.last_decoded_frame()==0 && movie.keyframe()==1);
    RegisteredFrameBuffers info{};
    info.buffer_count=2;info.frame_num=1;
    info.y_width=movie.plane_width(0);info.y_height=movie.plane_height(0);
    info.c_width=movie.plane_width(1);info.c_height=movie.plane_height(1);
    std::array<std::array<std::vector<std::uint8_t>,4>,2> storage;
    for(unsigned slot=0;slot<2;++slot)
        for(unsigned p=0;p<movie.plane_count();++p) {
            const auto w=movie.plane_width(p),h=movie.plane_height(p);
            auto& v=storage[slot][p];v.assign(w*h,slot==0?0xB7:0x80);
            auto& plane=info.slots[slot].planes[p];
            plane.allocate=1;plane.data=v.data();plane.pitch=w;
        }
    assert(movie.register_buffers(info));
    std::ofstream out(argv[2],std::ios::binary);
    assert(out);
    assert(movie.header().frame_count>=16);
    for(unsigned i=0;i<16;++i) {
        assert(movie.frame_number()==i+1);
        if(i==4) {
            assert(movie.pause(1) && movie.is_paused());
            assert(movie.pause(0) && !movie.is_paused());
        }
        auto result=movie.do_frame();
        if(result!=ResidentVideoSession::DecodeResult::frame) {
            std::cerr<<movie.error()<<"\n";return 1;
        }
        assert(info.frame_num==i%2 && movie.last_decoded_frame()==i+1);
        for(unsigned p=0;p<movie.plane_count();++p) {
            auto& v=storage[info.frame_num][p];
            out.write(reinterpret_cast<const char*>(v.data()),static_cast<std::streamsize>(v.size()));
        }
        const auto slot=info.frame_num;
        const auto before=storage[slot];
        const auto before_tick=tick;
        assert(movie.do_frame()==ResidentVideoSession::DecodeResult::already_decoded);
        assert(tick==before_tick && info.frame_num==slot && before==storage[slot]);
        if(i!=15)assert(movie.next_frame());
    }
    out.close();assert(out);
    movie.close();
    assert(movie.frame_number()==0 && movie.last_decoded_frame()==0);
    assert(!movie.next_frame());
    std::cout<<"Resident frame state + registered decode integrated PASS: 16 frames\n";
}
