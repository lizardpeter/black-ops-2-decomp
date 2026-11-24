#include "resident_video_session.h"
#include "resident_bootstrap.h"
#include "file_io_state.h"
#include <algorithm>
#include <cstring>
#include <utility>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include <exception>
#include <fstream>

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

// Actual host file and worker adapter for the original file I/O state
// routines. A single mutex serializes the OS file position and ring state;
// original callbacks execute under that lock. No full-file read.
struct ResidentVideoSession::StreamInput {
    explicit StreamInput(ResidentVideoSession& session):owner(session) {storage.configure(owner.alloc_,owner.free_);}
    ~StreamInput() {
        {std::lock_guard lock(mutex);stopping=true;}
        cv.notify_all();
        if(thread.joinable())thread.join();
    }
    ResidentVideoSession& owner;
    std::ifstream file;
    NativeAllocation storage;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopping=false;
    std::thread thread;
    void* io() {return reinterpret_cast<std::uint8_t*>(owner.handle_.data())+0x1F0;}
    static std::uint32_t tick() {
        return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    static int held_lock(void*) {return 1;} // outer real mutex owns this boundary
    static void seek(void* p,std::uint64_t offset) {
        auto& self=*static_cast<StreamInput*>(p);
        self.file.clear();self.file.seekg(static_cast<std::streamoff>(offset));
    }
    static std::uint32_t read_file(void* p,void* output,std::uint32_t count) {
        auto& self=*static_cast<StreamInput*>(p);
        self.file.read(static_cast<char*>(output),count);
        return static_cast<std::uint32_t>(self.file.gcount());
    }
    bool can_fill() {
        const auto* s=io();
        return !read<std::uint32_t>(s,0x80)&&!read<std::uint32_t>(s,0xB0)&&
            read<std::uint32_t>(s,0xF8)>=0x20000U &&
            read<std::uint32_t>(s,0xF4)<read<std::uint32_t>(s,0xEC);
    }
    static std::int64_t refill(void* state,void* ctx) {
        auto& self=*static_cast<StreamInput*>(ctx);
        return self.can_fill()?bink_research_file_refill(state,read_file,&self):0;
    }
    void start() {
        // Original open performs initial refill before handing back HBINK.
        {std::lock_guard lock(mutex);while(can_fill())if(refill(io(),this)<=0)break;}
        thread=std::thread([this] {
            std::unique_lock lock(mutex);
            for(;;) {
                cv.wait(lock,[&]{return stopping||can_fill();});
                if(stopping)return;
                if(refill(io(),this)<=0) {
                    // Failed read is terminal; avoid an unproductive spin.
                    put(io(),0x80,1U);
                }
                cv.notify_all();
                // Give foreground frame consumption a scheduling opportunity.
                lock.unlock();std::this_thread::yield();lock.lock();
            }
        });
    }
    bool read_pending() {
        auto* state=owner.handle_.data();
        if(!read<std::uint32_t>(state,0x138))return true;
        const auto offset=read<std::uint32_t>(state,0x1E8);
        const auto count=read<std::uint32_t>(state,0x13C);
        auto* dest=read<void*>(state,0x100);
        std::unique_lock lock(mutex);
        const auto got=bink_research_file_read_frame(io(),read<std::uint32_t>(state,0x1EC),
            offset,dest,count,seek,read_file,this);
        if(got!=count)return false;
        put(state,0x138,0U);put(state,0x598,offset+count);
        lock.unlock();cv.notify_all();return true;
    }
    std::uint32_t control(std::uint32_t flags) {
        std::lock_guard lock(mutex);
        const auto result=bink_research_file_control(io(),flags,refill,this);
        cv.notify_all();return result;
    }
};
ResidentVideoSession::ResidentVideoSession()=default;
ResidentVideoSession::~ResidentVideoSession(){close();}
bool ResidentVideoSession::fail(std::string reason) {
    failed_=true;error_=std::move(reason);return false;
}
void ResidentVideoSession::close() {
    // Join the real worker before releasing any borrowed/owned storage.
    worker_.reset();
    stream_.reset();
    // Do not otherwise touch the borrowed descriptor during close.
    opened_=failed_=false;
    info_=nullptr;timer_=nullptr;clock_ctx_=nullptr;
    header_={};handle_.clear();
    file_.clear();borrowed_=nullptr;packet_storage_.clear();file_size_=packet_offset_=0;error_.clear();
}
void ResidentVideoSession::set_memory_callbacks(Allocate alloc,Deallocate free_fn) {
    if(opened_)throw std::logic_error("movie already open");
    handle_.configure(alloc,free_fn);packet_storage_.configure(alloc,free_fn);
    alloc_=alloc;free_=free_fn;
}
bool ResidentVideoSession::open_borrowed(const std::uint8_t* file,std::size_t bytes,ResidentTimer timer,void* ctx) {
    close();
    HeaderPrefix h{};
    if(!file||!timer||bytes>256U*1024U*1024U || !validate_header_prefix(file,bytes,&h) ||
       h.magic!=0x694B4942U || h.audio_track_count || (h.flags&~kAlphaFlag) ||
       h.width<16 || h.width>1280 || h.height<16 || h.height>720 ||
       std::uint64_t(h.payload_bytes)+8!=bytes || h.largest_frame_bytes>262144)
        return fail("unsupported borrowed Store BIKi header");
    std::vector<FrameSpan> spans(h.frame_count);
    if(!read_video_only_index(file,bytes,spans.data(),spans.size(),&h) ||
       !spans.front().keyframe || spans.back().end!=bytes)return fail("invalid borrowed index");
    for(const auto& span:spans) {
        VideoPacketLayout layout{};
        if(!parse_video_packet({file+span.start,span.end-span.start},h.flags,layout))
            return fail("invalid borrowed video packet");
    }
    ResidentBootstrapLayout plan{};
    if(!resident_bootstrap_layout(h,plan,true))return fail("unsupported borrowed layout");
    header_=h;file_size_=bytes;timer_=timer;clock_ctx_=ctx;borrowed_=file;
    handle_.resize(plan.allocation_bytes/8);
    if(!init_resident_external(handle_.data(),plan.allocation_bytes,file,bytes,0x04104400U,timer_,clock_ctx_))
        return fail("borrowed bootstrap failed");
    opened_=true;return true;
}
bool ResidentVideoSession::open(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx) {
    return open_impl(std::move(file),timer,ctx,false,0);
}
bool ResidentVideoSession::open_preloaded_file(const char* path,std::uint32_t io_size,ResidentTimer timer,void* ctx) {
    close();
    if(!path)return fail("null movie filename");
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input)return fail("cannot open movie file");
    const auto size=input.tellg();
    if(size<44 || size>256*1024*1024)return fail("unsupported movie file size");
    std::vector<std::uint8_t> file(static_cast<std::size_t>(size));
    input.seekg(0);
    if(!input.read(reinterpret_cast<char*>(file.data()),size))
        return fail("cannot read complete movie");
    input.close();
    return open_impl(std::move(file),timer,ctx,true,io_size);
}

bool ResidentVideoSession::open_streaming_file(const char* path,std::uint32_t io_size,ResidentTimer timer,void* ctx) {
    close();
    if(!path||!timer||io_size>0x7FFE0000U)return fail("invalid streaming open");
    auto input=std::make_unique<StreamInput>(*this);
    input->file.open(path,std::ios::binary|std::ios::ate);
    if(!input->file)return fail("cannot open stream file");
    const auto size=input->file.tellg();
    if(size<44 || size>256*1024*1024)return fail("invalid streaming file size");
    file_size_=static_cast<std::size_t>(size);
    input->file.seekg(0);
    std::vector<std::uint8_t> prefix(44);
    if(!input->file.read(reinterpret_cast<char*>(prefix.data()),44))return fail("short stream header");
    HeaderPrefix h{};
    if(!validate_header_prefix(prefix.data(),prefix.size(),&h) || h.magic!=0x694B4942U ||
       h.audio_track_count || (h.flags&~kAlphaFlag) || h.width<16 || h.width>1280 ||
       h.height<16 || h.height>720 || h.frame_count!=h.frame_index_count ||
       std::uint64_t(h.payload_bytes)+8!=file_size_ || h.largest_frame_bytes>0x20000U)
        return fail("unsupported Store streaming header");
    const auto reserve=bink_research_file_io_reserve(io_size);
    if(reserve>=(h.payload_bytes/10U)*5U)
        return fail("original file mode would preload this movie");
    const auto index_bytes=4U*(h.frame_count+1U);
    prefix.resize(44U+index_bytes);
    if(prefix.size()>file_size_ || !input->file.read(reinterpret_cast<char*>(prefix.data()+44),index_bytes))
        return fail("short stream index");
    std::vector<FrameSpan> spans(h.frame_count);
    if(!read_video_only_index_prefix(prefix.data(),prefix.size(),file_size_,spans.data(),spans.size(),&h) ||
       !spans.front().keyframe || spans.back().end!=file_size_)
        return fail("invalid stream index");
    for(const auto& span:spans)
        if(span.end<=span.start || span.end-span.start>h.largest_frame_bytes)
            return fail("invalid stream packet span");
    ResidentBootstrapLayout plan{};
    if(!resident_bootstrap_layout(h,plan,true,index_bytes))return fail("unsupported stream handle layout");
    // Original 0x18F00 reserve: largest packet+0x500 rounded to
    // 32, then cache-line pad to 32 mod 1024. Followed by the ring reserve.
    const auto rounded=(h.largest_frame_bytes+0x51FU)&~31U;
    const auto scratch_bytes=rounded+((((1U-(rounded>>5))&31U))<<5);
    const auto allocation=reserve+scratch_bytes;
    handle_.resize(plan.allocation_bytes/8);
    input->storage.resize(allocation/8);
    std::memset(handle_.data(),0,plan.allocation_bytes);
    auto* state=reinterpret_cast<std::uint8_t*>(handle_.data());
    auto* owned_prefix=state+plan.init_region_offset+96;
    std::memcpy(owned_prefix,prefix.data(),prefix.size());
    auto* index=reinterpret_cast<std::uint32_t*>(owned_prefix+44);
    put(state,0,h.width);put(state,4,h.height);put(state,8,h.frame_count);
    put(state,0x14,h.rate_numerator);put(state,0x18,h.rate_denominator);
    put(state,0x20,0x01004400U|h.flags);put(state,0x24,h.flags);put(state,0x28,h.payload_bytes);
    const auto cols=(((h.width+15U)>>4)+15U)&~15U;
    put(state,0x110,cols);put(state,0x114,cols*((h.height+15U)>>4));
    put(state,0x118,h.largest_frame_bytes);put(state,0x11C,h.frame_index_count);
    put(state,0x134,1U);put(state,0x1B8,h.width);put(state,0x1BC,h.height);
    put(state,0x108,state+plan.track_ids_offset);put(state,0x198,state+plan.dummy_audio_offset);
    for(auto off:{0x1A0,0x1C0,0x1C8,0x1D0})put(state,off,index);
    put(state,0x228,state);
    constexpr std::size_t rings[]={0x4D8,0x4E0,0x4E8,0x4F0,0x4F8,0x500,0x508};
    for(unsigned i=0;i<7;++i)put(state,rings[i],state+plan.history_offsets[i]);
    put(state,0x510,plan.history_count);put(state,0x528,plan.accounted_bytes+allocation);
    put(state,0x560,h.rate_numerator);put(state,0x564,h.rate_denominator);put(state,0x59C,h.magic);
    auto* raw=reinterpret_cast<std::uint8_t*>(input->storage.data());
    put(state,0x5A0,raw+reserve);
    auto* io=state+0x1F0;
    put(io,0x38,state);put(io,0x48,&StreamInput::held_lock);put(io,0x68,&StreamInput::tick);
    put(io,0xD0,spans.front().start);put(io,0xF4,spans.front().start);
    if(bink_research_file_set_buffer(io,raw,reserve+8,static_cast<std::uint32_t>(file_size_)))
        return fail("original ring layout rejected");
    StreamInput::seek(input.get(),spans.front().start);
    if(!input->file)return fail("cannot seek first stream packet");
    header_=h;timer_=timer;clock_ctx_=ctx;
    if(bink_research_select_stream_frame(state,1,timer_,clock_ctx_))return fail("original streaming select failed");
    put(state,0x10,UINT32_MAX);
    try {input->start();}catch(const std::exception& e){return fail(e.what());}
    stream_=std::move(input);opened_=true;
    return true;
}
bool ResidentVideoSession::open_impl(std::vector<std::uint8_t> file,ResidentTimer timer,void* ctx,
                                     bool file_mode,std::uint32_t io_size) {
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
    header_=h;file_size_=file.size();
    timer_=timer;clock_ctx_=ctx;
    if(file_mode) {
        PreloadedFileLayout plan{};
        if(!preloaded_file_layout(file.data(),file.size(),io_size,plan))
            return fail("original file mode requires unresolved streaming path");
        packet_offset_=plan.first_packet;
        handle_.resize(plan.handle.allocation_bytes/sizeof(std::uint64_t));
        packet_storage_.resize(plan.payload_allocation/sizeof(std::uint64_t));
        if(!init_preloaded_file(handle_.data(),plan.handle.allocation_bytes,
            packet_storage_.data(),plan.payload_allocation,file.data(),file.size(),io_size,timer_,clock_ctx_))
            return fail("original-derived preloaded file bootstrap failed");
        // The original closes its file and owns only the copied index and
        // compressed payload. Release our temporary complete-file input.
    } else {
        ResidentBootstrapLayout plan{};
        if(!resident_bootstrap_layout(h,plan,true))return fail("unsupported original resident layout");
        file_=std::move(file);
        handle_.resize(plan.allocation_bytes/sizeof(std::uint64_t));
        if(!init_resident_external(handle_.data(),plan.allocation_bytes,file_.data(),file_.size(),
                                   0x04104400U,timer_,clock_ctx_))
            return fail("original-derived resident bootstrap failed");
    }
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
    const auto* base=packet_storage_.empty()?(borrowed_?borrowed_:file_.data()):
        reinterpret_cast<const std::uint8_t*>(packet_storage_.data());
    if(off>file_size_ || bytes>file_size_-off ||
       (stream_ ? (read<std::uint32_t>(state,0x138)!=0 || !packet) :
        (off<packet_offset_ || packet!=base+(off-packet_offset_)))) {
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
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
    if(bink_research_prepare_video_frame(state,timer_(clock_ctx_))!=1) {
        fail("original prepare state rejected frame");return DecodeResult::error;
    }
    if(io_lock.owns_lock())io_lock.unlock();
    if(stream_ && !stream_->read_pending()) {
        fail("stream packet read failed");return DecodeResult::error;
    }
    RegisteredDecodeReport report{};std::string reason;
    if(!decode_current_packet(report,reason)) {
        fail(std::move(reason));return DecodeResult::error;
    }
    if(stream_)io_lock.lock();
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
    if(stream_ && !stream_->read_pending()) {
        fail("stream packet read failed");return SubmitResult::error;
    }
    auto& queue=*worker_;
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
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
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
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
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
    if(stream_ ? bink_research_next_stream_frame(handle_.data(),timer_,clock_ctx_) :
        bink_research_next_resident_frame(handle_.data(),timer_,clock_ctx_))
        return fail("original next-frame state failed");
    return true;
}
bool ResidentVideoSession::realtime(RealtimeSnapshot& output,std::uint32_t frames) {
    if(!opened_ || failed_ || async_pending() || !last_decoded_frame())return false;
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
    return bink_research_get_realtime(handle_.data(),&output,frames,timer_,clock_ctx_)==0;
}
std::uint32_t ResidentVideoSession::control_background_io(std::uint32_t flags) {
    if(!opened_||async_pending())return UINT32_MAX;
    if(stream_)return stream_->control(flags);
    return bink_research_control_background_io(handle_.data(),flags);
}
bool ResidentVideoSession::pause(std::uint32_t value) {
    if(async_pending())return false;
    if(!opened_ || failed_)return fail("no resident movie");
    std::unique_lock<std::mutex> io_lock;
    if(stream_)io_lock=std::unique_lock(stream_->mutex);
    if(bink_research_pause_video_only(handle_.data(),value,timer_,clock_ctx_)!=value)
        return fail("original pause state failed");
    return true;
}
}
