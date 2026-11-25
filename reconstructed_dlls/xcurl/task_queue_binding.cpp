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
std::uint32_t TaskQueueBinding::submit_completion(void* queue,void* easy){
    return submit_worker(queue,easy,[](void* p,const void* s){xcurl_research_easy_complete(p,static_cast<const RequestServices*>(s));},&request_);
}
void TaskQueueBinding::adjust_global(std::uint32_t n){
    xcurl_research_engine_decrement(global_.engine,n,&request_);
}
}
