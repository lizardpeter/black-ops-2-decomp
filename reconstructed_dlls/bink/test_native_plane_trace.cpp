#include "video_plane.h"
#include "video_packet.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
int main(int argc,char** argv) {
    if(argc!=3)return 2;
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    using t6_bink::packet_le32;
    if(bytes.size()<52)return 3;
    const auto width=(packet_le32(bytes.data()+20)+7)&~7U;
    const auto height=(packet_le32(bytes.data()+24)+7)&~7U;
    const auto start=packet_le32(bytes.data()+44)&~1U,end=packet_le32(bytes.data()+48)&~1U;
    if(start>=end||end>bytes.size())return 4;
    std::vector<std::uint8_t> packet(bytes.begin()+start,bytes.begin()+end);
    t6_bink::VideoPacketLayout layout{};
    if(!t6_bink::parse_video_packet(packet,packet_le32(bytes.data()+36),layout))return 5;
    packet.resize(packet.size()+4);
    std::vector<std::uint8_t> pixels(width*height,0xB7),ref(width*height,0x80);
    BinkFirstLumaTrace trace{};
    const auto result=bink_research_decode_video_plane(packet.data(),static_cast<std::uint32_t>(packet.size()),
        layout.luma_start,static_cast<std::uint32_t>(packet.size()),width,height,
        pixels.data(),width,ref.data(),&trace);
    std::cout<<"PORTABILITY status="<<result<<" cursor="<<trace.input_words*4<<" luma_end="<<layout.luma_end
             <<" first_mode_count="<<trace.first_mode_count<<" first_submode_count="<<trace.first_submode_count<<"\n";
    std::ofstream out(argv[2],std::ios::binary);
    out.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
    return result;
}
