#include "transfer_binding.h"
#include "multi_lifecycle.h"
#include <cstring>
namespace t6_xcurl {
TransferBinding::TransferBinding(GlobalLifecycle& g,const PrepareServices& p,const MultiHostServices& h)
    :global_(g),prepare_(p),host_(h),
     scheduler_{enter,leave,start,progress,cancel,after_cancel,reset,event,wait_one,this},
     multi_{enter,leave,wait_one,wait_many,release_mutex,event,pending,active,cancel,after_cancel,progress,done,adjust,release,this}{}
TransferBinding& TransferBinding::self(void* c){return *static_cast<TransferBinding*>(c);}
void TransferBinding::enter(void* p,void* c){auto* r=self(c).prepare_.request;r->enter(p,r->context);}
void TransferBinding::leave(void* p,void* c){auto* r=self(c).prepare_.request;r->leave(p,r->context);}
void TransferBinding::start(void* p,void* c){
    auto& b=self(c);auto prepare=b.prepare_;
    // Read the current engine-owned queue at admission, not construction.
    std::memcpy(&prepare.engine_queue,static_cast<const std::uint8_t*>(b.global_.engine)+0x90,sizeof(void*));
    xcurl_bound_easy_begin(p,&prepare);
}
void TransferBinding::progress(void* p,void* c){xcurl_research_easy_progress(p,self(c).prepare_.request);}
void TransferBinding::cancel(void* p,std::uint32_t h,std::uint32_t code,void* c){xcurl_research_easy_cancel(p,h,code,self(c).prepare_.request);}
void TransferBinding::after_cancel(void* p,void* c){xcurl_research_easy_stop(p,self(c).prepare_.request);}
void TransferBinding::reset(void* p,void* c){auto& h=self(c).host_;h.reset_event(p,h.context);}
void TransferBinding::event(void* p,void* c){auto* r=self(c).prepare_.request;r->set_event(p,r->context);}
std::uint32_t TransferBinding::wait_one(void* p,std::uint32_t t,void* c){auto& h=self(c).host_;return h.wait_one(p,t,h.context);}
std::uint32_t TransferBinding::wait_many(std::uint32_t n,const void* p,std::uint32_t t,void* c){auto& h=self(c).host_;return h.wait_many(n,p,t,h.context);}
void TransferBinding::release_mutex(void* p,void* c){auto& h=self(c).host_;h.release_mutex(p,h.context);}
void TransferBinding::pending(void* p,void* c){auto& b=self(c);xcurl_bound_advance_pending(p,b.global_.engine,&b.scheduler_);}
void TransferBinding::active(void* p,void* c){auto& b=self(c);xcurl_research_advance_active(p,&b.scheduler_);}
std::uint8_t TransferBinding::done(void* p,void* c){return xcurl_research_easy_done(p,self(c).prepare_.request);}
void TransferBinding::adjust(std::uint32_t n,void* c){auto& b=self(c);xcurl_research_engine_decrement(b.global_.engine,n,b.prepare_.request);}
void TransferBinding::release(void* p,std::uint32_t t,void* c){auto* r=self(c).prepare_.request;r->release(p,t,r->context);}
std::uint32_t TransferBinding::add(void* m,void* e){return global_.engine?xcurl_research_multi_add(m,e,&multi_):1U;}
std::uint32_t TransferBinding::remove(void* m,void* e){return global_.engine?xcurl_research_multi_remove(m,e,&multi_):1U;}
std::uint32_t TransferBinding::perform(void* m,std::uint32_t* n){return global_.engine?xcurl_research_multi_perform_inner(m,n,&multi_):1U;}
std::uint32_t TransferBinding::poll(void* m,std::uint32_t t,std::uint32_t* n){return global_.engine?xcurl_research_multi_poll_inner(m,t,n,&multi_):1U;}
std::uint32_t TransferBinding::wait(void* m,std::uint32_t t,std::uint32_t* n){return global_.engine&&m?xcurl_research_multi_wait(m,t,n,&scheduler_):1U;}
std::uint32_t TransferBinding::wakeup(void* m){return global_.engine&&m?xcurl_research_multi_wakeup(m,&scheduler_):1U;}
void* TransferBinding::info_read(void* m,std::uint32_t* n){return global_.engine&&n?xcurl_research_multi_info_read_inner(m,n,&multi_):nullptr;}
}
