#pragma once
#include "global_lifecycle.h"
#include "gdk_interfaces.h"
#include "request_engine.h"
#include <map>
#include <mutex>
namespace t6_xcurl {
// Source-link bridge for the original manual easy task queue (B55C,
// A8F0, BA8C). The owner must drain queues before destroying easy handles,
// RequestServices, the engine or this adapter.
class TaskQueueBinding {
public:
    TaskQueueBinding(GlobalLifecycle& g,const RequestServices& r,const GdkQueryServices& d):global_(g),request_(r),gdk_(d){}
    // Native facade preallocates the original 0x28-byte completion node
    // before admission. No heap allocation is needed at completion time.
    void enable_reservations(){reserved_mode_=true;}
    bool reserve(void* easy,bool& created);
    void discard(void* easy);
    bool reservations_empty();
    std::uint32_t create(void**);
    std::uint8_t dispatch(void*,std::uint32_t,std::uint32_t);
    void cancel(void*);
    using Worker=void(*)(void*,const void*);
    std::uint32_t submit_worker(void* queue,void* payload,Worker,const void* services);
    std::uint32_t submit_completion(void*,void*);
    void adjust_global(std::uint32_t);
private:
    GlobalLifecycle& global_;
    const RequestServices& request_;
    const GdkQueryServices& gdk_;
    bool reserved_mode_=false;
    std::mutex reservation_lock_;
    std::map<void*,void*> reservations_;
    void* take(void*);
    void complete_reserved(void*);
};
}
