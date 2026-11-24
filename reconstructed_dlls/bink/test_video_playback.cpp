#include "video_playback.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <utility>

using t6_bink::VideoPlayback;
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream stream(argv[1],std::ios::binary);
    assert(stream);
    const std::vector<std::uint8_t> file((std::istreambuf_iterator<char>(stream)),{});
    VideoPlayback player;
    assert(player.decode_next()==VideoPlayback::Result::error);
    assert(!player.open({}));
    assert(player.plane(0).empty());
    assert(player.open(file));
    assert(player.frames_decoded()==0 && player.plane(0).empty());
    assert(player.decode_next()==VideoPlayback::Result::frame);
    assert(player.current_keyframe());
    std::array<std::vector<std::uint8_t>,3> first;
    for(unsigned p=0;p<3;++p) {
        const auto view=player.plane(p);
        assert(view.size()==player.plane_width(p)*player.plane_height(p));
        first[p].assign(view.begin(),view.end());
    }
    assert(player.plane(3).empty());
    assert(player.decode_next()==VideoPlayback::Result::frame);
    assert(player.reset() && player.frames_decoded()==0 && player.plane(0).empty());
    assert(player.decode_next()==VideoPlayback::Result::frame);
    for(unsigned p=0;p<3;++p)
        assert(std::equal(first[p].begin(),first[p].end(),player.plane(p).begin(),player.plane(p).end()));
    while(player.frames_decoded()<player.header().frame_count)
        assert(player.decode_next()==VideoPlayback::Result::frame);
    assert(player.decode_next()==VideoPlayback::Result::end);
    assert(player.decode_next()==VideoPlayback::Result::end);
    assert(!player.plane(0).empty());
    auto invalid=file;
    invalid[0]=0;
    assert(!player.open(std::move(invalid)) && player.plane(0).empty());
    assert(player.open(file) && player.decode_next()==VideoPlayback::Result::frame);
}
