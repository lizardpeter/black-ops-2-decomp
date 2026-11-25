// Native callback entrypoints for original 0x11F10 and 0x14A00;
// original task callbacks B4C0, C9C0, B800, C4A0 execute recovered source.
#include "callback_binding.h"
namespace t6_xcurl {
std::atomic<CallbackBinding*> CallbackBinding::active_{nullptr};
CallbackBinding::CallbackBinding(TaskQueueBinding& t,const StartServices& start,const StatusServices& status,
                                 const HeaderServices& h,const BodyServices& b,const UploadServices& u)
    :tasks_(t),start_(start),status_(status),headers_(h),body_(b),upload_(u){
    headers_.submit=submit_header;body_.submit=submit_body;
}
bool CallbackBinding::activate(){CallbackBinding* expected=nullptr;return active_.compare_exchange_strong(expected,this);}
void CallbackBinding::deactivate(){auto* expected=this;active_.compare_exchange_strong(expected,nullptr);}
void* CallbackBinding::start_token(){return reinterpret_cast<void*>(start_entry);}
void CallbackBinding::start_entry(void* async){
    auto* b=active_.load();if(b&&async)xcurl_bound_start_worker(async,&b->start_);
}
void CallbackBinding::status_entry(std::uintptr_t request,std::uint32_t status,const void* info,std::uint32_t length){
    // Disabled-ETW 14A00 forwards dwContext, status, info and length.
    auto* b=active_.load();if(b&&request)xcurl_bound_http_status(reinterpret_cast<void*>(request),status,info,length,&b->status_);
}
std::uint32_t CallbackBinding::submit_header(void* queue,void* easy,std::uint32_t id,void*){
    auto* b=active_.load();if(!b)return 0x8000ffffU;
    if(id==0)return b->tasks_.submit_worker(queue,easy,[](void* p,const void* s){xcurl_research_header_worker(p,static_cast<const HeaderServices*>(s));},&b->headers_);
    if(id==1)return b->tasks_.submit_worker(queue,easy,[](void* p,const void* s){xcurl_bound_upload_worker(p,static_cast<const UploadServices*>(s));},&b->upload_);
    return 0x80070057U;
}
std::uint32_t CallbackBinding::submit_body(void* queue,void* data,std::uint32_t id,void*){
    auto* b=active_.load();if(!b)return 0x8000ffffU;
    if(id==0)return b->tasks_.submit_worker(queue,data,[](void* p,const void* s){xcurl_research_deferred_read(p,static_cast<const BodyServices*>(s));},&b->body_);
    if(id==1)return b->tasks_.submit_worker(queue,data,[](void* p,const void* s){xcurl_research_body_worker(p,static_cast<const BodyServices*>(s));},&b->body_);
    return 0x80070057U;
}
}
