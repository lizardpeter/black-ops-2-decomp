// Reusable source playback coordinator; packet and plane contracts inherited
// from the original-derived differential-tested standalone research decoder.
#include "video_playback.h"
#include "video_plane.h"
#include <algorithm>
#include <utility>

namespace t6_bink {
namespace {
std::uint32_t le32(const std::uint8_t* p) {
    return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|
        (std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);
}
}
bool VideoPlayback::fail(std::string reason) {
    failed_=true;
    error_=std::move(reason);
    return false;
}
bool VideoPlayback::open(std::vector<std::uint8_t> file) {
    *this=VideoPlayback{};
    HeaderPrefix h{};
    if(file.size()>256U*1024U*1024U ||
       !validate_header_prefix(file.data(),file.size(),&h))
        return fail("invalid original Bink header");
    if(h.magic!=0x694B4942U || h.audio_track_count!=0 || h.flags!=0 ||
       h.width<16 || h.width>1280 || h.height<16 || h.height>720 ||
       std::uint64_t(h.payload_bytes)+8!=file.size() ||
       h.largest_frame_bytes>262144)
        return fail("not a supported Store BIKi video-only fixture");
    std::vector<FrameSpan> index(h.frame_count);
    if(!read_video_only_index(file.data(),file.size(),index.data(),index.size(),&h) ||
       !index.front().keyframe || index.back().end!=file.size())
        return fail("invalid video index");
    // Validate every packet span before any call into the source bitreader.
    for(const auto& span:index) {
        const auto bytes=span.end-span.start;
        if(bytes<8 || bytes>262144 || (bytes&3U))
            return fail("unsupported packet length");
        const auto split=le32(file.data()+span.start);
        if(split<=4 || split>=bytes || (split&3U))
            return fail("invalid plane split");
    }
    header_=h;
    file_=std::move(file);
    index_=std::move(index);
    opened_=true;
    return reset();
}
unsigned VideoPlayback::plane_width(unsigned p) const {
    return !opened_ || p>=3 ? 0 : ((p==0 ? header_.width : (header_.width+1)/2)+7U)&~7U;
}
unsigned VideoPlayback::plane_height(unsigned p) const {
    return !opened_ || p>=3 ? 0 : ((p==0 ? header_.height : (header_.height+1)/2)+7U)&~7U;
}
bool VideoPlayback::reset() {
    if(!opened_)return fail("no opened video");
    for(unsigned slot=0;slot<2;++slot)
        for(unsigned p=0;p<3;++p)
            buffers_[slot][p].assign(plane_width(p)*plane_height(p),slot==0?0xB7:0x80);
    next_=0;
    failed_=false;
    error_.clear();
    return true;
}
bool VideoPlayback::current_keyframe() const {
    return opened_ && !failed_ && next_!=0 && index_[next_-1].keyframe;
}
std::span<const std::uint8_t> VideoPlayback::plane(unsigned p) const {
    if(!opened_ || failed_ || !next_ || p>=3)return {};
    return std::span<const std::uint8_t>(buffers_[(next_-1)%2][p]);
}
VideoPlayback::Result VideoPlayback::decode_next() {
    if(!opened_ || failed_) {
        if(!failed_)fail("no opened video");
        return Result::error;
    }
    if(next_==header_.frame_count)return Result::end;
    const auto span=index_[next_];
    const auto bytes=span.end-span.start;
    const auto* packet=file_.data()+span.start;
    const auto split=le32(packet);
    auto& current=buffers_[next_%2];
    const auto& previous=buffers_[1-next_%2];
    std::uint32_t pos=4;
    for(unsigned p=0;p<3;++p) {
        if(p==1)pos=split;
        const auto limit=(next_==0 && p==0)?split:bytes;
        const auto w=plane_width(p),h=plane_height(p);
        BinkFirstLumaTrace trace{};
        const int status=bink_research_decode_video_plane(packet,bytes,pos,limit,
            w,h,current[p].data(),w,previous[p].data(),&trace);
        const auto cursor=4U*trace.input_words;
        if(status || cursor<=pos || cursor>limit || (p==0 && cursor!=split) ||
           (p==2 && cursor!=bytes)) {
            fail("frame="+std::to_string(next_)+" plane="+std::to_string(p)+
                 " status="+std::to_string(status)+" block="+std::to_string(trace.error_x)+
                 ","+std::to_string(trace.error_y)+" mode="+std::to_string(trace.mode)+
                 " submode="+std::to_string(trace.submode)+" input_offset="+std::to_string(cursor));
            return Result::error;
        }
        pos=cursor;
    }
    ++next_;
    return Result::frame;
}
}
