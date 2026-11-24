// Standalone BIKi video-only research decoder. All codec code is linked
// from original-machine-derived reconstructed_dlls/bink/*.cpp.
// No original Bink DLL, Unicorn, FFmpeg, SDK or other codec is loaded.
// Only trusted, validated BO2 fixtures with the tested plane format should
// be used: the inner original-derived bitreader is not hardened.
#include "../../reconstructed_dlls/bink/bink_index.h"
#include "../../reconstructed_dlls/bink/video_plane.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
std::uint32_t le32(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
        (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
int fail(const char* reason) {
    std::cerr<<"Bink source decoder: "<<reason<<"\n";
    return 1;
}
}

int main(int argc,char** argv) {
    if(argc!=3) return fail("usage: bink_decode_video INPUT.bik OUTPUT.yuv");
    const std::filesystem::path input_path=argv[1], output_path=argv[2];
    std::error_code ec;
    if(std::filesystem::exists(output_path,ec))
        return fail("output exists; refusing to overwrite");
    std::ifstream input(input_path,std::ios::binary|std::ios::ate);
    if(!input)return fail("cannot open input");
    const auto size=input.tellg();
    if(size<44 || size>std::streamoff(256*1024*1024))
        return fail("unsupported input size");
    std::vector<std::uint8_t> file(static_cast<std::size_t>(size));
    input.seekg(0);
    if(!input.read(reinterpret_cast<char*>(file.data()),size))
        return fail("cannot read input");

    t6_bink::HeaderPrefix header{};
    if(!t6_bink::validate_header_prefix(file.data(),file.size(),&header))
        return fail("invalid original Bink header");
    if(header.magic!=0x694B4942U || header.audio_track_count!=0 ||
        header.flags!=0 || header.width<32 || header.width>512 ||
        header.height<32 || header.height>512 ||
        (header.width&31U) || (header.height&31U) ||
        std::uint64_t(header.payload_bytes)+8!=file.size() ||
        header.largest_frame_bytes>32768)
        return fail("not a supported 32-aligned BIKi video-only fixture");
    std::vector<t6_bink::FrameSpan> index(header.frame_count);
    if(!t6_bink::read_video_only_index(file.data(),file.size(),
            index.data(),index.size(),&header) ||
        !index.front().keyframe || index.back().end!=file.size())
        return fail("invalid video index");

    // Match the independently verified two-buffer research configuration.
    // Skipped tail blocks preserve previous contents of the destination slot.
    const std::array<unsigned,3> widths{header.width,header.width/2,header.width/2};
    const std::array<unsigned,3> heights{header.height,header.height/2,header.height/2};
    std::array<std::array<std::vector<std::uint8_t>,3>,2> frames;
    for(unsigned slot=0;slot<2;++slot)
        for(unsigned i=0;i<3;++i)
            frames[slot][i].assign(widths[i]*heights[i],slot==0?0xB7:0x80);
    // Decode to a temporary output. Never advertise a completed video
    // or replace a preexisting destination if a frame fails validation.
    auto partial=output_path;
    partial += ".partial";
    if(std::filesystem::exists(partial,ec))
        return fail("partial output already exists; refusing to overwrite");
    std::ofstream output(partial,std::ios::binary);
    if(!output)return fail("cannot open partial output");
    const auto abort_decode=[&](const char* reason) {
        output.close();
        std::error_code ignored;
        std::filesystem::remove(partial,ignored);
        return fail(reason);
    };
    for(std::uint32_t f=0;f<header.frame_count;++f) {
        const auto span=index[f];
        const auto bytes=span.end-span.start;
        if(bytes<8 || bytes>32768 || (bytes&3U)!=0)
            return abort_decode("unsupported packet length");
        const auto* packet=file.data()+span.start;
        const std::uint32_t split=le32(packet);
        if(split<=4 || split>=bytes || (split&3U)!=0)
            return abort_decode("invalid plane split");
        auto& current=frames[f%2];
        const auto& previous=frames[1-f%2];
        std::uint32_t pos=4;
        for(unsigned p=0;p<3;++p) {
            if(p==1)pos=split;
            const std::uint32_t limit=(f==0&&p==0)?split:bytes;
            const auto w=widths[p], h=heights[p];
            BinkFirstLumaTrace trace{};
            const int status=bink_research_decode_video_plane(packet,bytes,pos,limit,
                w,h,current[p].data(),w,previous[p].data(),&trace);
            const auto next=4U*trace.input_words;
            if(status || next<=pos || next>limit || (p==0&&next!=split) ||
                (p==2&&next!=bytes)) {
                std::cerr<<"frame="<<f<<" plane="<<p<<" status="<<status
                    <<" block="<<trace.error_x<<","<<trace.error_y
                    <<" mode="<<trace.mode<<" submode="<<trace.submode
                    <<" input_offset="<<next<<"\n";
                return abort_decode("unsupported or incomplete plane");
            }
            pos=next;
        }
        for(const auto& plane:current)
            output.write(reinterpret_cast<const char*>(plane.data()),
                         static_cast<std::streamsize>(plane.size()));
        if(!output)return abort_decode("output write failed");
    }
    output.close();
    if(!output)return abort_decode("output close failed");
    if(std::filesystem::exists(output_path,ec))
        return abort_decode("output appeared during decode");
    std::filesystem::rename(partial,output_path,ec);
    if(ec)return abort_decode("cannot finalize output");
    std::cout<<"Native Bink video-only decode complete: "<<header.frame_count
        <<" frames, "<<header.width<<"x"<<header.height
        <<", rate "<<header.rate_numerator<<"/"<<header.rate_denominator
        <<", native planar output Y/C1/C2 (original decoder order)\n";
    return 0;
}
