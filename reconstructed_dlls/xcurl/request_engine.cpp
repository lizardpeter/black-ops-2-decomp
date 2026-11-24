// Original XCurl Store x64, SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Disabled ETW paths; OS operations, task-queue creation/dispatch and logging are explicit boundaries.
#include "request_engine.h"
#include <cstring>
#include <bit>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T> T get(P p,unsigned o){T v;std::memcpy(&v,p+o,sizeof(v));return v;}
template<class T> void put(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
P ptr(P p,unsigned o){return get<P>(p,o);}
void clear(P p,unsigned o){put<P>(p,o,nullptr);}
void free_at(P p,unsigned o,unsigned tag,const RequestServices* s){if(auto* v=ptr(p,o)){s->release(v,tag,s->context);clear(p,o);}}
std::uint32_t hr(std::uint32_t e){return std::bit_cast<std::int32_t>(e)>0?(e&0xffffU)|0x80070000U:e;}
bool failed(std::uint32_t e){return (e&0x80000000U)!=0;}
void unlink(P n){put(ptr(n,0),8,ptr(n,8));put(ptr(n,8),0,ptr(n,0));put(n,0,n);put(n,8,n);}
void push_front(P h,P n){put(n,0,ptr(h,0));put(n,8,h);put(ptr(h,0),8,n);put(h,0,n);}
void push_back(P h,P n){put(n,0,h);put(n,8,ptr(h,8));put(ptr(h,8),0,n);put(h,8,n);}
}
extern "C" std::uint32_t xcurl_research_easy_start_engine(void* easy,const RequestServices* s){ // B55C
    auto* e=static_cast<P>(easy);auto* event=s->create_event(s->context);put(e,0x360,event);
    std::uint32_t status;
    if(event){void* queue=nullptr;std::memcpy(&queue,e+0x350,8);status=s->create_queue(&queue,s->context);put(e,0x350,queue);if(failed(status))s->diagnostic(e,0x20710,status,s->context);}
    else{status=hr(s->last_error(s->context));s->diagnostic(e,0x20740,status,s->context);}
    return status;
}
extern "C" void xcurl_research_easy_progress(void* easy,const RequestServices* s){ // A8F0
    auto* e=static_cast<P>(easy);while(s->queue_dispatch(ptr(e,0x350),1,0,s->context)){}
}
extern "C" void xcurl_research_easy_cancel(void* easy,std::uint32_t h,std::uint32_t code,const RequestServices* s){ // CA60
    auto* e=static_cast<P>(easy);s->enter(e+0x188,s->context);
    if(!failed(get<std::uint32_t>(e,0x228))){put(e,0x220,code);put(e,0x228,h);}
    s->leave(e+0x188,s->context);
}
extern "C" std::uint8_t xcurl_research_easy_done(void* easy,const RequestServices* s){ // B6B4
    auto* e=static_cast<P>(easy);s->enter(e+0x10,s->context);s->enter(e+0x230,s->context);
    auto done=get<std::uint32_t>(e,0x258)==0;s->leave(e+0x230,s->context);s->leave(e+0x10,s->context);return done;
}
extern "C" void xcurl_research_request_reset(void* request,const RequestServices* s){ // 10AC4
    auto* p=static_cast<P>(request);s->enter(p,s->context);
    if(auto* v=ptr(p,0x48)){s->close_http(v,s->context);clear(p,0x48);}
    if(auto* v=ptr(p,0x40)){s->release_connection(v,s->context);clear(p,0x40);}
    free_at(p,0x90,15,s);clear(p,0x98);free_at(p,0xb0,16,s);clear(p,0xa8);free_at(p,0x38,7,s);
    put(p,0x28,0U);put(p,0xb8,std::uint8_t(0));clear(p,0xc8);put(p,0xd0,0U);put(p,0xd4,std::uint16_t(0));put(p,0xd6,std::uint8_t(0));put(p,0xdc,0U);
    s->leave(p,s->context);
}
extern "C" void xcurl_research_easy_complete(void* easy,const RequestServices* s){ // A6B0
    auto* e=static_cast<P>(easy);s->enter(e+0x10,s->context);s->enter(e+0x188,s->context);
    free_at(e,0x340,14,s);free_at(e,0x348,14,s);auto* m=ptr(e,0x310);
    put(e,0x48,get<std::uint32_t>(e,0x220));put(e,0x40,e);put(e,0x38,1U);
    s->leave(e+0x188,s->context);xcurl_research_request_reset(e+0x230,s);s->leave(e+0x10,s->context);
    s->enter(m+0x10,s->context);auto* h=m+0x50; s->enter(m+0x10,s->context);auto* n=ptr(h,0);while(n&&n!=h&&n!=e)n=ptr(n,0);
    const bool found=n==e;if(found)unlink(e);s->leave(m+0x10,s->context);
    if(found){s->enter(m+0x10,s->context);push_front(m+0x60,e);s->leave(m+0x10,s->context);auto* msg=static_cast<P>(s->allocate(0x28,12,s->context));
        // Original unconditionally dereferences this allocation: non-null allocator contract for this path.
        std::memcpy(msg+0x10,e+0x38,24);push_back(m+0x78,msg);
        put(m,0x88,get<std::uint32_t>(m,0x88)+1);auto active=get<std::uint32_t>(m,0x3c)-1;put(m,0x3c,active);
        if(!active)s->set_event(ptr(m,0xb0),s->context);
    }
    s->leave(m+0x10,s->context);if(found)s->adjust_global(1,s->context);
}
extern "C" void xcurl_research_engine_decrement(void* engine,std::uint32_t amount,const RequestServices* s){ // 16DE4
    auto* p=static_cast<P>(engine);s->enter(p,s->context);put(p,0x40,get<std::uint32_t>(p,0x40)-amount);s->leave(p,s->context);
}
extern "C" std::uint32_t xcurl_research_http_read(void* request,void* buffer,std::uint32_t size,const RequestServices* s){ // 12124
    return s->read(ptr(static_cast<P>(request),0x50),buffer,size,s->context)?0:hr(s->last_error(s->context));
}
extern "C" std::uint32_t xcurl_research_http_receive(void* request,const RequestServices* s){ // 127A8
    return s->receive(ptr(static_cast<P>(request),0x50),s->context)?0:hr(s->last_error(s->context));
}
extern "C" std::uint32_t xcurl_research_http_send(void* request,const void* body,std::uint64_t size,std::uint64_t total,const RequestServices* s){ // 12D4C
    auto* p=static_cast<P>(request);auto* e=ptr(p,0x30);s->enter(e+0x188,s->context);put(e,0x200,total);s->leave(e+0x188,s->context);
    if(s->send(ptr(p,0x50),body,static_cast<std::uint32_t>(size),static_cast<std::uint32_t>(total),p,s->context)){
        e=ptr(p,0x30);if(e[0x176]&&body)s->debug_data(e,4,body,size,s->context);put(p,0xa0,size);return 0;
    }
    auto status=hr(s->last_error(s->context));s->diagnostic(ptr(p,0x30),0x21418,status,s->context);return status;
}
extern "C" std::uint32_t xcurl_research_http_register(void* request,const RequestServices* s){ // 13038
    auto* p=static_cast<P>(request);std::uint32_t error_site;
    if(!s->set_context(ptr(p,0x50),p,s->context))error_site=0x21678;
    else if(s->set_callback(ptr(p,0x50),UINT32_MAX,s->context)==UINTPTR_MAX)error_site=0x21648;
    else return 0;
    auto status=hr(s->last_error(s->context));s->diagnostic(ptr(p,0x30),error_site,status,s->context);return status;
}
extern "C" void xcurl_research_request_close(void* request,const RequestServices* s){ // 13C30
    auto* p=static_cast<P>(request);s->enter(p,s->context);const auto close=get<std::uint32_t>(p,0x28)==2;
    if(close)put(p,0x28,5U);s->leave(p,s->context);if(close)s->close_http(ptr(p,0x50),s->context);
}
extern "C" void xcurl_research_request_stop(void* request,const RequestServices* s){ // 1395C
    auto* p=static_cast<P>(request);s->enter(p,s->context);auto state=get<std::uint32_t>(p,0x28);
    if(state==0)put(p,0x28,3U);
    else if(state==1){put(p,0x28,4U);s->cancel_task(p+0x58,s->context);}
    s->leave(p,s->context);if(state==2)xcurl_research_request_close(p,s);
}
extern "C" void xcurl_research_easy_notify(void* easy,const RequestServices* s){ // B90C / 18750
    auto* e=static_cast<P>(easy);if(auto* m=ptr(e,0x310))s->set_event(ptr(m,0xa0),s->context);
    s->set_event(ptr(e,0x360),s->context);
}
extern "C" void xcurl_research_easy_schedule_completion(void* easy,const RequestServices* s){ // BA8C
    auto* e=static_cast<P>(easy);s->enter(e+0x10,s->context);auto status=s->submit_completion(ptr(e,0x350),e,s->context);
    if(failed(status)){s->diagnostic(e,0x20920,status,s->context);xcurl_research_easy_complete(e,s);}
    else xcurl_research_easy_notify(e,s);
    s->leave(e+0x10,s->context);
}
extern "C" void xcurl_research_request_finish(void* request,const RequestServices* s){ // 13B14
    auto* p=static_cast<P>(request);s->enter(p,s->context);bool finish=get<std::uint32_t>(p,0x28)<6;
    if(finish)put(p,0x28,6U);s->leave(p,s->context);if(finish)xcurl_research_easy_schedule_completion(ptr(p,0x30),s);
}
