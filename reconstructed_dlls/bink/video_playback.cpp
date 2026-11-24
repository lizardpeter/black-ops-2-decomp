// Reusable source playback coordinator; packet and plane contracts inherited
// from the original-derived differential-tested standalone research decoder.
#include "video_playback.h"
#include "video_registered.h"
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
    RegisteredFrameBuffers info{};
    info.buffer_count=2;
    info.y_width=plane_width(0);
    info.y_height=plane_height(0);
    info.c_width=plane_width(1);
    info.c_height=plane_height(1);
    info.frame_num=1-next_%2;
    for(unsigned slot=0;slot<2;++slot)
        for(unsigned p=0;p<plane_count();++p) {
            auto& plane=info.slots[slot].planes[p];
            plane.allocate=1;
            plane.data=buffers_[slot][p].data();
            plane.pitch=plane_width(p);
        }
    RegisteredDecodeReport report{};
    const int status=decode_registered_packet({packet,bytes},header_.flags,info,report);
    if(status || report.destination_slot!=next_%2) {
        fail("frame="+std::to_string(next_)+" plane="+std::to_string(report.plane)+
             " status="+std::to_string(status)+" block="+std::to_string(report.trace.error_x)+
             ","+std::to_string(report.trace.error_y)+" mode="+std::to_string(report.trace.mode)+
             " submode="+std::to_string(report.trace.submode)+" input_offset="+std::to_string(report.next_offset));
        return Result::error;
    }
    ++next_;
    return Result::frame;
}
}
