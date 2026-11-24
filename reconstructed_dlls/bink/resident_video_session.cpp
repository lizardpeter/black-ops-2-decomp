#include "resident_video_session.h"
#include "resident_bootstrap.h"
#include <cstring>
#include <utility>

namespace {
template<class T> T read(const void* h,std::size_t off) {
    T v{};
    std::memcpy(&v,static_cast<const std::uint8_t*>(h)+off,sizeof(v));
    return v;
}
template<class T> void put(void* h,std::size_t off,T v) {
    std::memcpy(static_cast<std::uint8_t*>(h)+off,&v,sizeof(v));
}
unsigned round8(unsigned x) {return (x+7U)&~7U;}
}
namespace t6_bink {
bool ResidentVideoSession::fail(std::string reason) {
    failed_=true;error_=std::move(reason);return false;
}
void ResidentVideoSession::close() {
    // Do not touch the borrowed descriptor during close.
    opened_=failed_=false;
    info_=nullptr;timer_=nullptr;clock_ctx_=nullptr;
    header_={};handle_.clear();
    file_.clear();error_.clear();
}
bool ResidentVideoSession::open(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx) {
    close();
    HeaderPrefix h{};
    if(!timer || file.size()>256U*1024U*1024U ||
       !validate_header_prefix(file.data(),file.size(),&h) ||
       h.magic!=0x694B4942U || h.audio_track_count || (h.flags&~kAlphaFlag) ||
       h.width<16 || h.width>1280 || h.height<16 || h.height>720 ||
       std::uint64_t(h.payload_bytes)+8!=file.size() || h.largest_frame_bytes>262144)
        return fail("unsupported resident Store BIKi header");
    std::vector<FrameSpan> spans(h.frame_count);
    if(!read_video_only_index(file.data(),file.size(),spans.data(),spans.size(),&h) ||
       !spans.front().keyframe || spans.back().end!=file.size())
        return fail("invalid resident index");
    for(const auto& span:spans) {
        VideoPacketLayout layout{};
        if(!parse_video_packet({file.data()+span.start,span.end-span.start},h.flags,layout))
            return fail("invalid resident video packet");
    }
    ResidentBootstrapLayout plan{};
    if(!resident_bootstrap_layout(h,plan))return fail("unsupported original resident layout");
    header_=h;file_=std::move(file);
    timer_=timer;clock_ctx_=ctx;
    handle_.resize(plan.allocation_bytes/sizeof(std::uint64_t));
    if(!init_resident_external(handle_.data(),plan.allocation_bytes,file_.data(),file_.size(),
                               0x04000400U|h.flags,timer_,clock_ctx_))
        return fail("original-derived resident bootstrap failed");
    opened_=true;
    return true;
}
unsigned ResidentVideoSession::plane_width(unsigned p) const {
    return !opened_||p>=plane_count()?0:round8((p==0||p==3)?header_.width:(header_.width+1U)/2);
}
unsigned ResidentVideoSession::plane_height(unsigned p) const {
    return !opened_||p>=plane_count()?0:round8((p==0||p==3)?header_.height:(header_.height+1U)/2);
}
bool ResidentVideoSession::register_buffers(RegisteredFrameBuffers& info) {
    if(!opened_ || failed_ || last_decoded_frame()!=0 || info_ || info.buffer_count!=2 ||
       info.frame_num>1 || info.y_width!=plane_width(0) || info.y_height!=plane_height(0) ||
       info.c_width!=plane_width(1) || info.c_height!=plane_height(1))
        return fail("invalid registered frame descriptor");
    for(unsigned p=0;p<plane_count();++p) {
        const auto& a=info.slots[0].planes[p];const auto& b=info.slots[1].planes[p];
        if(!a.data || !b.data || a.data==b.data || a.pitch!=b.pitch ||
           a.pitch<plane_width(p) || a.pitch>16384U)
            return fail("invalid registered plane storage");
    }
    info_=&info;
    put(handle_.data(),0xD0,info_);
    return true;
}
std::uint32_t ResidentVideoSession::frame_number() const {
    return handle_.empty()?0:read<std::uint32_t>(handle_.data(),0x0C);
}
std::uint32_t ResidentVideoSession::last_decoded_frame() const {
    return handle_.empty()?0:read<std::uint32_t>(handle_.data(),0x56C);
}
std::uint32_t ResidentVideoSession::keyframe() const {
    return handle_.empty()?0:read<std::uint32_t>(handle_.data(),0x140);
}
bool ResidentVideoSession::is_paused() const {
    return !handle_.empty()&&read<std::uint32_t>(handle_.data(),0x128)!=0;
}
ResidentVideoSession::DecodeResult ResidentVideoSession::do_frame() {
    if(!opened_ || failed_ || !info_) {
        fail("no ready registered resident video");return DecodeResult::error;
    }
    if(frame_number()==last_decoded_frame())return DecodeResult::already_decoded;
    auto* state=handle_.data();
    if(bink_research_prepare_video_frame(state,timer_(clock_ctx_))!=1) {
        fail("original prepare state rejected frame");return DecodeResult::error;
    }
    const auto* packet=read<const std::uint8_t*>(state,0x100);
    const auto bytes=read<std::uint32_t>(state,0x13C);
    const auto off=read<std::uint32_t>(state,0x1E8);
    if(off>file_.size() || bytes>file_.size()-off || packet!=file_.data()+off) {
        fail("resident frame pointer out of bounds");return DecodeResult::error;
    }
    RegisteredDecodeReport report{};
    if(decode_registered_packet({packet,bytes},header_.flags,*info_,report)) {
        fail("resident packet decode failed at frame="+std::to_string(frame_number())+
             " plane="+std::to_string(report.plane)+" status="+std::to_string(report.status));
        return DecodeResult::error;
    }
    if(bink_research_finish_video_frame(state,timer_(clock_ctx_),timer_,clock_ctx_) ||
       info_->frame_num!=report.destination_slot || last_decoded_frame()!=frame_number()) {
        fail("original completion state rejected frame");return DecodeResult::error;
    }
    return DecodeResult::frame;
}
bool ResidentVideoSession::next_frame() {
    // Integration safety: only allow sequentially decoded reference frames.
    // The underlying original BinkNextFrame transcription is less restrictive.
    if(!opened_ || failed_ || !info_ || frame_number()!=last_decoded_frame())
        return fail("advance requires completed current frame");
    if(bink_research_next_resident_frame(handle_.data(),timer_,clock_ctx_))
        return fail("original next-frame state failed");
    return true;
}
bool ResidentVideoSession::realtime(RealtimeSnapshot& output,std::uint32_t frames) {
    if(!opened_ || failed_ || !last_decoded_frame())return false;
    return bink_research_get_realtime(handle_.data(),&output,frames,timer_,clock_ctx_)==0;
}
bool ResidentVideoSession::pause(std::uint32_t value) {
    if(!opened_ || failed_)return fail("no resident movie");
    if(bink_research_pause_video_only(handle_.data(),value,timer_,clock_ctx_)!=value)
        return fail("original pause state failed");
    return true;
}
}
