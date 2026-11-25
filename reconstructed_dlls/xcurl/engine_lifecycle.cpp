// Original Store XCurl 0x164D4, 0x16C04, 0x165D0, 0x166EC,
// 0x17044, 0x170F4, 0x16EC4, 0x16B60, 0x169FC, 0x9BA0 and 0x13D50.
// Disabled ETW path. OS callbacks and the telemetry object's lifecycle remain
// explicit service boundaries, never calls into the retail binary.
#include "engine_lifecycle.h"
#include <bit>
#include <cstring>
#include <initializer_list>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(const void* p,unsigned o){return get<P>(p,o);}
void self(P head){put(head,0,head);put(head,8,head);}
// Splice an entire circular list at the tail, preserving its order.
void splice(P dest,P src){if(ptr(src,0)==src)return;auto* first=ptr(src,0);auto* last=ptr(src,8);auto* tail=ptr(dest,8);put(tail,0,first);put(first,8,tail);put(last,0,dest);put(dest,8,last);self(src);}
bool failed(std::uint32_t v){return (v&0x80000000U)!=0;}
std::uint32_t hr(std::uint32_t v){return std::bit_cast<std::int32_t>(v)>0?(v&0xffffU)|0x80070000U:v;}
}
extern "C" void* xcurl_research_engine_construct(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;
    s->initialize_cs(g,c);put<P>(g,0x28,nullptr);put(g,0x40,0U);put(g,0x44,0U);put(g,0x48,std::uint16_t(0));self(g+0x30);
    s->initialize_cs(g+0x50,c);self(g+0x78);
    for(auto o:{0x88U,0x90U,0x98U,0xa0U})put<P>(g,o,nullptr);
    return g;
}
extern "C" std::uint32_t xcurl_research_engine_initialize(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;
    auto status=s->telemetry_create(reinterpret_cast<void**>(g+0x98),c);if(failed(status))return status;
    status=xcurl_research_gdk_task_queue_create(1,1,reinterpret_cast<void**>(g+0x90),s->gdk);if(failed(status))return status;
    auto* mutex=s->create_mutex(c);put(g,0x28,mutex);if(!mutex)return hr(s->last_error(c));
    status=xcurl_research_gdk_network_register_connectivity(ptr(g,0x90),g,s->connectivity_callback,g+0x88,s->gdk);if(failed(status))return status;
    return s->register_app_state(s->app_state_callback,g,reinterpret_cast<void**>(g+0xa0),c);
}
extern "C" void xcurl_research_engine_destroy(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;
    if(auto* h=ptr(g,0xa0))s->unregister_app_state(h,c);
    xcurl_research_session_cache_clear(g+0x50,s->sessions);
    xcurl_research_gdk_network_unregister_connectivity(get<std::uint64_t>(g,0x88),1,s->gdk);
    if(auto* h=ptr(g,0x28))s->close_handle(h,c);
    if(auto* q=ptr(g,0x90)){xcurl_research_gdk_task_queue_terminate(q,1,nullptr,nullptr,s->gdk);xcurl_research_gdk_task_queue_close(q,s->gdk);}
    if(auto* t=ptr(g,0x98))s->telemetry_destroy(t,c);
    xcurl_research_session_cache_clear(g+0x50,s->sessions);s->delete_cs(g+0x50,c);s->delete_cs(g,c);
}
extern "C" void xcurl_research_engine_connectivity(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;s->enter(g,c);
    if(!g[0x48]&&!g[0x49]){
        alignas(8) std::uint8_t hint[16]{};
        auto status=xcurl_research_gdk_network_get_connectivity_hint(hint,s->gdk);
        if(!failed(status)&&hint[12]){g[0x49]=1;auto* head=g+0x30;for(auto* m=ptr(head,0);m&&m!=head;m=ptr(m,0))s->set_event(ptr(m,0xa0),c);}
    }
    s->leave(g,c);
}
extern "C" void xcurl_research_engine_set_limit(void* engine,std::uint32_t limit,const EngineServices* s){
    auto c=s->context;s->enter(engine,c);put(engine,0x44,limit);s->leave(engine,c);
}
extern "C" void xcurl_research_engine_suspend(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;
    s->wait_one(ptr(g,0x28),UINT32_MAX,c);alignas(8) std::uint8_t temporary[16];self(temporary);
    s->enter(g,c);put(g,0x48,std::uint16_t(1));auto* head=g+0x30;
    for(auto* m=ptr(head,0);m&&m!=head;m=ptr(m,0))xcurl_research_abort_all(m,s->scheduler);
    splice(temporary,head);s->leave(g,c);
    for(auto* m=ptr(temporary,0);m&&m!=temporary;m=ptr(m,0))s->wait_one(ptr(m,0xb0),UINT32_MAX,c);
    s->enter(g,c);splice(head,temporary);xcurl_research_session_cache_clear(g+0x50,s->sessions);s->leave(g,c);s->release_mutex(ptr(g,0x28),c);
}
extern "C" void xcurl_research_engine_resume(void* engine,const EngineServices* s){
    auto* g=static_cast<P>(engine);auto c=s->context;s->wait_one(ptr(g,0x28),UINT32_MAX,c);s->enter(g,c);g[0x48]=0;
    auto* head=g+0x30;for(auto* m=ptr(head,0);m&&m!=head;m=ptr(m,0)){s->enter(m+0x10,c);m[0xc0]=0;s->leave(m+0x10,c);}
    s->leave(g,c);xcurl_research_engine_connectivity(g,s);s->release_mutex(ptr(g,0x28),c);
}
extern "C" void xcurl_research_engine_app_state(std::uint8_t quiesced,void* engine,const EngineServices* s){
    if(quiesced)xcurl_research_engine_suspend(engine,s);else xcurl_research_engine_resume(engine,s);
}
extern "C" std::uint32_t xcurl_research_engine_create(GlobalLifecycle* global,const EngineServices* s){
    auto c=s->context;auto* g=s->allocate(engine_bytes,11,c);if(!g)return 27;
    std::memset(g,0,engine_bytes);xcurl_research_engine_construct(g,s);auto status=xcurl_research_engine_initialize(g,s);
    if(failed(status)){xcurl_research_engine_destroy(g,s);s->release(g,11,c);}else global->engine=g;
    return status;
}
extern "C" std::uint32_t xcurl_research_engine_bootstrap(GlobalLifecycle* global,std::uint32_t limit,std::uint64_t* frequency,const EngineServices* s){
    auto c=s->context;if(failed(s->initialize_runtime(c)))return 2;if(global->engine)return 0;
    // Original 0x169FC returns positive 27 on allocation failure and 0x9BA0
    // mistakenly treats it as a successful HRESULT. Do not dereference null.
    auto status=xcurl_research_engine_create(global,s);if(failed(status)||!global->engine)return 2;
    xcurl_research_engine_set_limit(global->engine,limit,s);s->query_frequency(frequency,c);return 0;
}
extern "C" std::uint32_t xcurl_research_verify_server_certificate(void* request,const GdkQueryServices* s){
    auto* metadata=ptr(request,0x98);if(!metadata)return 0x80072f89U;
    return xcurl_research_gdk_network_verify_certificate(ptr(request,0x50),metadata,s);
}
