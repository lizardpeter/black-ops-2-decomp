#include "video_packet.h"
#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace t6_bink;
    std::array<std::uint8_t,32> packet{};
    VideoPacketLayout l{};
    packet[0]=12;
    assert(parse_video_packet(packet,0,l));
    assert(l.alpha_start==0 && l.alpha_end==0 && l.luma_start==4 &&
           l.luma_end==12 && l.end==32);
    packet[12]=12;
    assert(parse_video_packet(packet,kAlphaFlag,l));
    assert(l.alpha_start==4 && l.alpha_end==12 && l.luma_start==16 &&
           l.luma_end==24 && l.end==32);
    const auto valid=l;
    packet[12]=4;
    assert(!parse_video_packet(packet,kAlphaFlag,l));
    assert(l.luma_end==valid.luma_end);
    packet[12]=20;
    assert(!parse_video_packet(packet,kAlphaFlag,l)); // no chroma partition
    packet[0]=31;
    assert(!parse_video_packet(packet,kAlphaFlag,l));
    packet[0]=28;
    assert(!parse_video_packet(packet,kAlphaFlag,l));
    packet[0]=0;
    assert(!parse_video_packet(packet,0,l));
    packet[0]=12;
    packet[12]=12;
    assert(!parse_video_packet(packet,0x200000,l));
    assert(!parse_video_packet(std::span(packet).first(7),0,l));
    assert(!parse_video_packet(std::span(packet).first(31),0,l));
}
