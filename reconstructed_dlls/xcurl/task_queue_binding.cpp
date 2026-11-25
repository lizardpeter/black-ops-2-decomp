#include "task_queue_binding.h"
#include <new>
#include <memory>
namespace t6_xcurl {
namespace {
struct Completion {
    void* payload;
    TaskQueueBinding::Worker worker;
    const void* services;
};
// Original A6B0 uses the easy context and does not branch on the task
// callback's cancellation flag. Keep that behavior on both paths.
void complete(void* context,bool){
    std::unique_ptr<Completion> task(static_cast<Completion*>(context));
    task->worker(task->payload,task->services);
}
}
std::uint32_t TaskQueueBinding::create(void** out){
    return xcurl_research_gdk_task_queue_create(0,0,out,&gdk_);
}
std::uint8_t TaskQueueBinding::dispatch(void* queue,std::uint32_t port,std::uint32_t timeout){
    return xcurl_research_gdk_task_queue_dispatch(queue,port,timeout,&gdk_);
}
void TaskQueueBinding::cancel(void* block){xcurl_research_gdk_async_cancel(block,&gdk_);}
std::uint32_t TaskQueueBinding::submit_worker(void* queue,void* payload,Worker worker,const void* services){
    // The host carries a service pointer in addition to the original easy
    // context. On success the callback owns it, on failure the caller owns
    // it and BA8C executes the existing synchronous fallback.
    auto* task=new(std::nothrow) Completion{payload,worker,services};
    if(!task)return 0x8007000eU;
    auto status=xcurl_research_gdk_task_queue_submit(queue,1,task,reinterpret_cast<void*>(complete),&gdk_);
    if(status&0x80000000U)delete task;
    return status;
}
bool TaskQueueBinding::reserve(void* easy,bool& created){
    std::lock_guard lock(reservation_lock_);created=false;
    if(reservations_.contains(easy))return true;
    auto* node=request_.allocate(0x28,12,request_.context);if(!node)return false;
    try{reservations_.emplace(easy,node);}catch(const std::bad_alloc&){request_.release(node,12,request_.context);return false;}
    created=true;return true;
}
void* TaskQueueBinding::take(void* easy){
    std::lock_guard lock(reservation_lock_);auto it=reservations_.find(easy);if(it==reservations_.end())return nullptr;
    auto* node=it->second;reservations_.erase(it);return node;
}
void TaskQueueBinding::discard(void* easy){if(auto* node=take(easy))request_.release(node,12,request_.context);}
bool TaskQueueBinding::reservations_empty(){std::lock_guard lock(reservation_lock_);return reservations_.empty();}
void TaskQueueBinding::complete_reserved(void* easy){xcurl_bound_easy_complete(easy,take(easy),&request_);}
std::uint32_t TaskQueueBinding::submit_completion(void* queue,void* easy){
    if(reserved_mode_){
        auto status=submit_worker(queue,easy,[](void* p,const void* s){const_cast<TaskQueueBinding*>(static_cast<const TaskQueueBinding*>(s))->complete_reserved(p);},this);
        if(status&0x80000000U){
            request_.diagnostic(easy,0x20920,status,request_.context);
            complete_reserved(easy); // BA8C inline fallback, using reserved storage
        }
        return 0; // completion already ran or was queued; do not invoke retail fallback twice
    }
    return submit_worker(queue,easy,[](void* p,const void* s){xcurl_research_easy_complete(p,static_cast<const RequestServices*>(s));},&request_);
}
void TaskQueueBinding::adjust_global(std::uint32_t n){
    xcurl_research_engine_decrement(global_.engine,n,&request_);
}
}
