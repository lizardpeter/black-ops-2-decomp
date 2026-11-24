// Integration adapter for recovered source, not a transcription of all
// 0x17970 branches. Expose only Store video-only external-buffer paths.
#include "bink_native_api.h"
#include "bink_open_options.h"
#include "resident_video_session.h"
#include "file_io_state.h"
#include "async_lifecycle.h"
#include <thread>
#include <deque>
#include <array>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <exception>

namespace t6_bink {
extern Allocate g_allocate;
extern Deallocate g_deallocate;
}
namespace {
using namespace t6_bink;
struct Entry {
    ResidentVideoSession movie;
    RegisteredFrameBuffers* registered=nullptr;
};
// Platform adapter: original lifecycle mask and zero-message stop semantics,
// with real numbered host worker threads and a FIFO. Completed decode
// messages are retained by each session (not the original global queue).
struct HostPool {
    struct Slot {
        std::mutex mutex;
        std::condition_variable cv;
        std::deque<std::function<void()>> queue;
        std::thread thread;
        bool stopping=false;
        ~Slot() {stop();if(thread.joinable())thread.join();}
        void stop() {
            {std::lock_guard lock(mutex);stopping=true;}
            cv.notify_all();
        }
        bool start() {
            if(thread.joinable())return false;
            stopping=false;
            try {thread=std::thread([this] {
                std::unique_lock lock(mutex);
                for(;;) {
                    cv.wait(lock,[&]{return stopping||!queue.empty();});
                    if(queue.empty()&&stopping)return;
                    auto task=std::move(queue.front());queue.pop_front();
                    lock.unlock();task();lock.lock();
                }
            });return true;}catch(...){return false;}
        }
        bool submit(std::function<void()> job) {
            std::lock_guard lock(mutex);
            if(stopping||!thread.joinable()||queue.size()>=80)return false;
            queue.push_back(std::move(job));cv.notify_one();return true;
        }
    };
    AsyncLifecycle state{};
    std::array<Slot,8> slots;
    static int init(std::uint32_t,unsigned,void*) {return 1;} // in-place host queue storage
    static void destroy(std::uint32_t,unsigned,void*) {} // queue storage owned by slot
    static int launch(std::uint32_t n,void* ctx) {
        return static_cast<HostPool*>(ctx)->slots[n].start();
    }
    static int request(std::uint32_t n,std::uintptr_t msg,void* ctx) {
        if(msg)return 0;
        static_cast<HostPool*>(ctx)->slots[n].stop();return 1;
    }
    static int join(std::uint32_t n,void* ctx) {
        auto& slot=static_cast<HostPool*>(ctx)->slots[n];
        if(!slot.thread.joinable())return 0;
        slot.thread.join();return 1;
    }
};
HostPool pool;
std::mutex registry_mutex;
std::unordered_map<void*,std::unique_ptr<Entry>> registry;
void report_lifecycle() {
    switch(pool.state.error_rva) {
    case 0x5F530:BinkSetError("Out of range thread number.");break;
    case 0x5F550:BinkSetError("Already loaded on this thread number.");break;
    case 0x5F578:BinkSetError("Invalid thread number.");break;
    case 0x5F590:BinkSetError("Wait not queued for this thread.");break;
    default:break;
    }
}
std::uint32_t tick(void*) {
    return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
Entry* find(void* h) {
    auto i=registry.find(h);return i==registry.end()?nullptr:i->second.get();
}
bool ensure_buffers(void* h,Entry& e) {
    RegisteredFrameBuffers* info=nullptr;
    std::memcpy(&info,static_cast<const std::uint8_t*>(h)+0xD0,sizeof(info));
    if(!info) {BinkSetError("no registered frame buffers");return false;}
    if(e.registered) {
        if(e.registered==info)return true;
        BinkSetError("changing registered buffers is unsupported");return false;
    }
    if(!e.movie.register_buffers(*info)) {BinkSetError(e.movie.error().c_str());return false;}
    e.registered=info;return true;
}
void report(Entry& e,const char* fallback) {
    BinkSetError(e.movie.error().empty()?fallback:e.movie.error().c_str());
}
}
namespace t6_bink::internal {
void* open_decoder(void* name,const OpenOptions* o,std::uint32_t flags) {
    try {
        std::lock_guard lock(registry_mutex);
        BinkSetError("");
        if(!name||!o||o->track_count||o->file_offset||o->secondary_offset||
           (o->frame_rate_number && o->frame_rate_number!=UINT32_MAX)||
           (o->simulate && o->simulate!=UINT32_MAX)||
           (flags!=0x04104400U && flags!=0x01104400U)) {
            BinkSetError("unsupported Store native Bink open options");return nullptr;
        }
        auto e=std::make_unique<Entry>();
        e->movie.set_memory_callbacks(g_allocate,g_deallocate);
        bool ok=false;
        if(flags==0x04104400U) {
            // As original BinkOpen, caller supplies readable header and
            // the entire header-declared resident buffer for its lifetime.
            const auto* bytes=static_cast<const std::uint8_t*>(name);
            std::uint32_t length=0;std::memcpy(&length,bytes+4,4);
            ok=e->movie.open_borrowed(bytes,std::uint64_t(length)+8,tick,nullptr);
        } else {
            const auto* path=static_cast<const char*>(name);
            std::ifstream file(path,std::ios::binary|std::ios::ate);
            if(!file) {BinkSetError("cannot open movie");return nullptr;}
            const auto size=file.tellg();file.close();
            if(size<44 || size>256*1024*1024 || o->io_size>0x7FFE0000U) {
                BinkSetError("unsupported file size or I/O reserve");return nullptr;
            }
            const auto payload=static_cast<std::uint32_t>(size)-8;
            if(bink_research_file_io_reserve(o->io_size)>=(payload/10U)*5U)
                ok=e->movie.open_preloaded_file(path,o->io_size,tick,nullptr);
            else ok=e->movie.open_streaming_file(path,o->io_size,tick,nullptr);
        }
        if(!ok) {report(*e,"native open failed");return nullptr;}
        auto* h=e->movie.native_handle();registry.emplace(h,std::move(e));return h;
    } catch(const std::exception& e) {BinkSetError(e.what());return nullptr;}
}
std::uint64_t decode_frame_plane(void* h,std::uint32_t flags) {
    try {
        std::lock_guard lock(registry_mutex);
        auto* e=find(h);if(!e)return 0;
        if(flags!=0x303U||!ensure_buffers(h,*e))return 0;
        using R=ResidentVideoSession::DecodeResult;
        const auto r=e->movie.do_frame();
        if(r!=R::frame&&r!=R::already_decoded)report(*e,"native decode rejected");
        return 0;
    } catch(const std::exception& e) {BinkSetError(e.what());return 0;}
}
}
extern "C" void BinkClose(void* h) {
    std::unique_ptr<Entry> owned;
    {std::lock_guard lock(registry_mutex);auto i=registry.find(h);
     if(i==registry.end())return;owned=std::move(i->second);registry.erase(i);}
    // Join worker and free original-layout allocations outside registry lock.
    owned.reset();
}
extern "C" int BinkDoFrameAsync(void* h,std::uint32_t first,std::uint32_t second) {
    try {
        std::lock_guard lock(registry_mutex);
        auto* e=find(h);if(!e)return 0;
        if(first!=second||first>=8) {BinkSetError("only same-worker submission recovered");return 0;}
        if(!ensure_buffers(h,*e))return 0;
        using R=ResidentVideoSession::SubmitResult;
        if(!(pool.state.running&(1U<<first))) {BinkSetError("Invalid thread number.");return 0;}
        const auto r=e->movie.do_frame_async(first,[first](std::function<void()> job) {
            return pool.slots[first].submit(std::move(job));
        });
        if(r==R::submitted)return 1;
        if(r==R::error)report(*e,"native async submission rejected");
        return 0;
    } catch(const std::exception& e) {BinkSetError(e.what());return 0;}
}
extern "C" int BinkDoFrameAsyncWait(void* h,std::int32_t timeout) {
    std::lock_guard lock(registry_mutex);auto* e=find(h);if(!e)return 1;
    using R=ResidentVideoSession::WaitResult;
    auto r=e->movie.wait_async(timeout);
    if(r==R::frame||r==R::idle)return 1;
    if(r==R::error)report(*e,"native async wait rejected");
    return 0;
}
extern "C" void BinkNextFrame(void* h) {
    std::lock_guard lock(registry_mutex);auto* e=find(h);
    if(e&&!e->movie.next_frame()&&!e->movie.async_pending())report(*e,"native advance rejected");
}
extern "C" std::uint32_t BinkPause(void* h,std::uint32_t value) {
    std::lock_guard lock(registry_mutex);auto* e=find(h);if(!e)return 0;
    if(!e->movie.pause(value)) {report(*e,"native pause rejected");return 0;}return value;
}
extern "C" void BinkGetRealtime(void* h,RealtimeSnapshot* out,std::uint32_t frames) {
    std::lock_guard lock(registry_mutex);auto* e=find(h);
    if(e&&out&&!e->movie.realtime(*out,frames))report(*e,"native realtime rejected");
}
extern "C" std::uint32_t BinkControlBackgroundIO(void* h,std::uint32_t flags) {
    std::lock_guard lock(registry_mutex);auto* e=find(h);
    return e?e->movie.control_background_io(flags):UINT32_MAX;
}

extern "C" int BinkStartAsyncThread(std::uint32_t n,void*) {
    std::lock_guard lock(registry_mutex);
    const auto r=bink_research_start_worker(&pool.state,n,HostPool::init,HostPool::destroy,HostPool::launch,&pool);
    if(!r)report_lifecycle();return r;
}
extern "C" int BinkRequestStopAsyncThread(std::uint32_t n) {
    std::lock_guard lock(registry_mutex);
    const auto r=bink_research_stop_worker(&pool.state,n,HostPool::request,&pool);
    if(!r)report_lifecycle();return r;
}
extern "C" int BinkWaitStopAsyncThread(std::uint32_t n) {
    std::lock_guard lock(registry_mutex);
    // Do not join a live, non-stopping thread even when the original
    // stop-requested bit from an earlier lifecycle remains set.
    if(n<8 && (pool.state.running&(1U<<n))) {BinkSetError("Wait not queued for this thread.");return 0;}
    const auto r=bink_research_join_worker(&pool.state,n,HostPool::join,HostPool::destroy,&pool);
    if(!r)report_lifecycle();return r;
}
