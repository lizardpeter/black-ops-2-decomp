#include "video_registered.h"
#include <array>
#include <cassert>

int main() {
    using namespace t6_bink;
    RegisteredDecodeReport report{};
    RegisteredFrameBuffers info{};
    std::array<std::uint8_t,32> packet{};
    assert(bink_research_decode_registered_packet(nullptr,0,0,&info,&report)==1);
    assert(bink_research_decode_registered_packet(packet.data(),packet.size(),0,&info,&report)==1);
    packet[0]=12;
    assert(bink_research_decode_registered_packet(packet.data(),packet.size(),0,&info,&report)==2);
    info.buffer_count=2;
    info.y_width=32;info.y_height=32;
    info.c_width=16;info.c_height=16;
    info.frame_num=1;
    // A null/missing registered plane must fail before entering entropy code.
    assert(bink_research_decode_registered_packet(packet.data(),packet.size(),0,&info,&report)==2);
    std::array<std::uint8_t,1024> shared{};
    for(auto& slot:info.slots)
        for(unsigned p=0;p<3;++p) {
            slot.planes[p].data=shared.data();
            slot.planes[p].pitch=p==0?32:16;
        }
    assert(bink_research_decode_registered_packet(packet.data(),packet.size(),0,&info,&report)==2);
    assert(report.plane==0 && report.destination_slot==0);
}
