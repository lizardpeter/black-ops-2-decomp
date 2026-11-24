#include "resident_video_session.h"
#include "resident_bootstrap.h"
#include <cstring>
#include <utility>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include <exception>

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
// Actual host-thread adapter for the original tagged work and completion
// messages. One persistent worker per resident session; source dispatcher
// and wait own all pending bits, descriptor swaps and finish state.
// The worker never changes the caller's handle or slot selection.
struct ResidentVideoSession::Worker {
    explicit Worker(ResidentVideoSession& session):owner(session),thread([this]{run();}) {}
    ~Worker() {
        {std::lock_guard lock(mutex);stopping=true;}
        cv.notify_all();
        thread.join();
    }
    ResidentVideoSession& owner;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopping=false,queued=false,running=false,ready=false;
    std::uint32_t id=0;
    std::uintptr_t tag=0;
    RegisteredDecodeReport report{};
    std::string error;
    bool success=false;
    char api_error[256]{};
    std::uintptr_t wait_entry=0;
    std::thread thread;
    void run() {
        std::unique_lock lock(mutex);
        for(;;) {
            cv.wait(lock,[&]{return stopping||queued;});
            if(stopping&&!queued)return;
            queued=false;running=true;
            lock.unlock();
            RegisteredDecodeReport local{};
            std::string reason;
            bool ok=false;
            try {ok=owner.decode_current_packet(local,reason);}
            catch(const std::exception& e) {reason=e.what();}
            catch(...) {reason="unexpected native worker failure";}
            lock.lock();
            report=local;error=std::move(reason);success=ok;
            running=false;ready=true;cv.notify_all();
        }
    }
    static int enqueue(std::uint32_t worker,std::uintptr_t message,void* ctx) {
        auto& self=*static_cast<Worker*>(ctx);
        std::lock_guard lock(self.mutex);
        if(self.queued||self.running||self.ready||self.stopping||
           (message&7U)!=6U)return 0;
        // Do not wake until original submit has stored its pending word.
        self.id=worker;self.tag=message;return 1;
    }
    void launch() {
        {std::lock_guard lock(mutex);queued=true;success=false;error.clear();}
        cv.notify_one();
    }
    static int receive(std::uint32_t worker,std::int32_t timeout,
                       AsyncCompletion* completion,const char** error,void* ctx) {
        auto& self=*static_cast<Worker*>(ctx);
        std::unique_lock lock(self.mutex);
        if(worker!=self.id)return 0;
        if(!self.ready && (self.queued||self.running)) {
            if(timeout<0)self.cv.wait(lock,[&]{return self.ready||self.stopping;});
            else if(timeout>0)self.cv.wait_for(lock,std::chrono::milliseconds(timeout),
                                              [&]{return self.ready||self.stopping;});
        }
        if(!self.ready)return 0;
        completion->tagged_handle=self.tag&~std::uintptr_t(6);
        completion->result=self.success?0:-1;
        self.ready=false;
        if(error)*error=nullptr;
        return 1;
    }
    static void notify(std::uint32_t,void*) {} // no other handle in this adapter
};
ResidentVideoSession::ResidentVideoSession()=default;
ResidentVideoSession::~ResidentVideoSession(){close();}
bool ResidentVideoSession::fail(std::string reason) {
    failed_=true;error_=std::move(reason);return false;
}
void ResidentVideoSession::close() {
    // Join the real worker before releasing any borrowed/owned storage.
    worker_.reset();
    // Do not otherwise touch the borrowed descriptor during close.
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
bool ResidentVideoSession::decode_current_packet(RegisteredDecodeReport& report,std::string& reason) const {
    const auto* state=handle_.data();
    const auto* packet=read<const std::uint8_t*>(state,0x100);
    const auto bytes=read<std::uint32_t>(state,0x13C);
    const auto off=read<std::uint32_t>(state,0x1E8);
    if(off>file_.size() || bytes>file_.size()-off || packet!=file_.data()+off) {
        reason="resident frame pointer out of bounds";return false;
    }
    if(decode_registered_packet({packet,bytes},header_.flags,*info_,report)) {
        reason="resident packet decode failed at frame="+std::to_string(frame_number())+
             " plane="+std::to_string(report.plane)+" status="+std::to_string(report.status);
        return false;
    }
    return true;
}
bool ResidentVideoSession::async_pending() const {
    return !handle_.empty()&&(read<std::uint32_t>(handle_.data(),0x120)||
                              read<std::uint32_t>(handle_.data(),0x124));
}
ResidentVideoSession::DecodeResult ResidentVideoSession::do_frame() {
    if(async_pending())return DecodeResult::busy;
    if(!opened_ || failed_ || !info_) {
        fail("no ready registered resident video");return DecodeResult::error;
    }
    if(frame_number()==last_decoded_frame())return DecodeResult::already_decoded;
    auto* state=handle_.data();
    if(bink_research_prepare_video_frame(state,timer_(clock_ctx_))!=1) {
        fail("original prepare state rejected frame");return DecodeResult::error;
    }
    RegisteredDecodeReport report{};std::string reason;
    if(!decode_current_packet(report,reason)) {
        fail(std::move(reason));return DecodeResult::error;
    }
    if(bink_research_finish_video_frame(state,timer_(clock_ctx_),timer_,clock_ctx_) ||
       info_->frame_num!=report.destination_slot || last_decoded_frame()!=frame_number()) {
        fail("original completion state rejected frame");return DecodeResult::error;
    }
    return DecodeResult::frame;
}
ResidentVideoSession::SubmitResult ResidentVideoSession::do_frame_async(std::uint32_t worker) {
    if(async_pending())return SubmitResult::busy;
    if(!opened_ || failed_ || !info_ || worker>=8) {
        fail("no ready resident async video or invalid worker");return SubmitResult::error;
    }
    if(frame_number()==last_decoded_frame())return SubmitResult::already_decoded;
    try {
        if(!worker_)worker_=std::make_unique<Worker>(*this);
    } catch(const std::exception& e) {
        fail(e.what());return SubmitResult::error;
    }
    // Resident mode has no allocated dirty map (0xD8); no allocator
    // boundary is needed. The real dispatch stores the pending word.
    auto& queue=*worker_;
    auto result=bink_research_submit_async(handle_.data(),worker,worker,timer_,clock_ctx_,
        nullptr,nullptr,Worker::enqueue,&queue,queue.api_error,&queue.wait_entry,
        reinterpret_cast<std::uintptr_t>(&bink_research_wait_async));
    if(result!=1) {
        fail("original async submission rejected frame");return SubmitResult::error;
    }
    queue.launch();
    return SubmitResult::submitted;
}
ResidentVideoSession::WaitResult ResidentVideoSession::wait_async(std::int32_t timeout) {
    if(!opened_||failed_)return WaitResult::error;
    if(!async_pending())return WaitResult::idle;
    if(!worker_) {fail("missing native async worker");return WaitResult::error;}
    auto& queue=*worker_;
    const auto mask=1U<<queue.id;
    const int result=bink_research_wait_async(handle_.data(),timeout,mask,mask,timer_,clock_ctx_,
        Worker::receive,Worker::notify,&queue,queue.api_error);
    if(result==0)return WaitResult::pending;
    if(result!=1 || !queue.success || info_->frame_num!=queue.report.destination_slot ||
       last_decoded_frame()!=frame_number()) {
        fail(queue.error.empty()?"original async completion rejected frame":queue.error);
        return WaitResult::error;
    }
    return WaitResult::frame;
}
bool ResidentVideoSession::next_frame() {
    if(async_pending())return false;
    // Integration safety: only allow sequentially decoded reference frames.
    // The underlying original BinkNextFrame transcription is less restrictive.
    if(!opened_ || failed_ || !info_ || frame_number()!=last_decoded_frame())
        return fail("advance requires completed current frame");
    if(bink_research_next_resident_frame(handle_.data(),timer_,clock_ctx_))
        return fail("original next-frame state failed");
    return true;
}
bool ResidentVideoSession::realtime(RealtimeSnapshot& output,std::uint32_t frames) {
    if(!opened_ || failed_ || async_pending() || !last_decoded_frame())return false;
    return bink_research_get_realtime(handle_.data(),&output,frames,timer_,clock_ctx_)==0;
}
bool ResidentVideoSession::pause(std::uint32_t value) {
    if(async_pending())return false;
    if(!opened_ || failed_)return fail("no resident movie");
    if(bink_research_pause_video_only(handle_.data(),value,timer_,clock_ctx_)!=value)
        return fail("original pause state failed");
    return true;
}
}
