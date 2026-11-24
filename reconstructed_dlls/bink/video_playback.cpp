// Reusable source playback coordinator; packet and plane contracts inherited
// from the original-derived differential-tested standalone research decoder.
#include "video_playback.h"
#include "video_plane.h"
#include <algorithm>
#include <utility>

namespace t6_bink {
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
    if(h.magic!=0x694B4942U || h.audio_track_count!=0 || (h.flags&~kAlphaFlag) ||
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
        VideoPacketLayout layout{};
        if(!parse_video_packet({file.data()+span.start,bytes},h.flags,layout))
            return fail("invalid video plane partition");
    }
    header_=h;
    file_=std::move(file);
    index_=std::move(index);
    opened_=true;
    return reset();
}
unsigned VideoPlayback::plane_width(unsigned p) const {
    return p>=plane_count() ? 0 : (((p==0 || p==3) ? header_.width : (header_.width+1)/2)+7U)&~7U;
}
unsigned VideoPlayback::plane_height(unsigned p) const {
    return p>=plane_count() ? 0 : (((p==0 || p==3) ? header_.height : (header_.height+1)/2)+7U)&~7U;
}
bool VideoPlayback::reset() {
    if(!opened_)return fail("no opened video");
    for(unsigned slot=0;slot<2;++slot)
        for(unsigned p=0;p<plane_count();++p)
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
    if(!opened_ || failed_ || !next_ || p>=plane_count())return {};
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
    VideoPacketLayout layout{};
    if(!parse_video_packet({packet,bytes},header_.flags,layout)) {
        fail("invalid video plane partition");
        return Result::error;
    }
    auto& current=buffers_[next_%2];
    const auto& previous=buffers_[1-next_%2];
    // Original coordinator decodes A (if present), Y, C1, C2, whereas
    // the caller-facing frame descriptor uses Y/C1/C2/A storage order.
    const bool alpha=plane_count()==4;
    const std::array<unsigned,4> order{3,0,1,2};
    std::uint32_t pos=alpha?layout.alpha_start:layout.luma_start;
    for(unsigned i=alpha?0U:1U;i<order.size();++i) {
        const unsigned p=order[i];
        if(p==0)pos=layout.luma_start;
        if(p==1)pos=layout.luma_end;
        const auto limit=bytes;
        const auto w=plane_width(p),h=plane_height(p);
        BinkFirstLumaTrace trace{};
        const int status=bink_research_decode_video_plane(packet,bytes,pos,limit,
            w,h,current[p].data(),w,previous[p].data(),&trace);
        const auto cursor=4U*trace.input_words;
        if(status || cursor<=pos || cursor>limit || (p==3 && cursor!=layout.alpha_end) ||
           (p==0 && cursor!=layout.luma_end) ||
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
