// Standalone BIKi video-only research decoder. All codec code is linked
// from original-machine-derived reconstructed_dlls/bink/*.cpp.
// No original Bink DLL, Unicorn, FFmpeg, SDK or other codec is loaded.
// Only trusted, validated BO2 fixtures with the tested plane format should
// be used: the inner original-derived bitreader is not hardened.
#include "../../reconstructed_dlls/bink/video_playback.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <utility>

namespace {
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

    t6_bink::VideoPlayback playback;
    if(!playback.open(std::move(file)))return fail(playback.error().c_str());
    const auto& header=playback.header();
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
    while(true) {
        const auto result=playback.decode_next();
        if(result==t6_bink::VideoPlayback::Result::end)break;
        if(result!=t6_bink::VideoPlayback::Result::frame)
            return abort_decode(playback.error().c_str());
        for(unsigned p=0;p<3;++p) {
            const auto plane=playback.plane(p);
            output.write(reinterpret_cast<const char*>(plane.data()),
                         static_cast<std::streamsize>(plane.size()));
        }
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
