#include "handle_binding.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
struct Host {
    unsigned locks=0,cs=0,allocs=0,frees=0,events=0,closed=0,queries=0,releases=0,queues=0,term=0,dispatches=0,errors=0;
    bool fail_queue=false;
    alignas(8) std::array<std::uint8_t,0xa8> engine{};
    std::array<void*,30> slots{};
    struct Object{void** vtable;Host* h;} object{slots.data(),this};
    RequestServices request{};EngineServices es{};ApiBinding api{};
    GdkQueryServices gdk{};SchedulerServices scheduler{};EasyLifecycleServices easy{};MultiLifecycleServices multi{};
    Host();
};
Host& host(void* p){return *static_cast<Host*>(p);}
Host& obj(void* p){return *static_cast<Host::Object*>(p)->h;}
void enter(void*,void* c){++host(c).locks;}
void leave(void*,void* c){assert(host(c).locks);--host(c).locks;}
void init_cs(void*,void* c){++host(c).cs;}
void del_cs(void*,void* c){assert(host(c).cs);--host(c).cs;}
void* allocate(std::size_t n,std::uint32_t,void* c){++host(c).allocs;return std::malloc(n);}
void release(void* p,std::uint32_t,void* c){assert(p);++host(c).frees;std::free(p);}
void* create_event(void* c){++host(c).events;return c;}
void* create_multi_event(std::uint8_t,std::uint8_t,void* c){return create_event(c);}
void close(void* p,void* c){assert(p==c);++host(c).closed;}
void event(void*,void*){}
std::uint32_t wait(void*,std::uint32_t,void*){return 0;}
std::uint32_t last_error(void*){return 8;}
std::uint64_t tick(void*){return 42;}
void error(std::uint32_t code,std::uint32_t,void* c){assert(code);++host(c).errors;}
void diag(void*,std::uint32_t,std::uint32_t,void*){}
std::uint32_t query(const void*,const void*,void** out,void* c){++host(c).queries;*out=&host(c).object;return 0;}
std::uint32_t release_interface(void* p){++obj(p).releases;return 1;}
void thread_assert(void*){}
std::uint32_t terminate(void* p,void* q,std::uint8_t wait,void*,void*){assert(q==&obj(p)&&!wait);++obj(p).term;return 0;}
std::uint8_t dispatch(void* p,void* q,std::uint32_t port,std::uint32_t timeout){auto& h=obj(p);assert(q==&h&&port==h.dispatches%2&&!timeout);++h.dispatches;return h.dispatches%4==1;}
void queue_close(void* p,void* q){auto& h=obj(p);assert(q==&h&&h.queues);--h.queues;}
std::uint32_t queue_create(void** out,void* c){auto& h=host(c);if(h.fail_queue)return 0x80004005U;++h.queues;*out=c;return 0;}
Host::Host(){
    auto* g=engine.data();put(g,0x28,static_cast<void*>(this));put(g,0x30,g+0x30);put(g,0x38,g+0x30);
    es.initialize_cs=init_cs;es.delete_cs=del_cs;es.allocate=allocate;es.release=release;es.context=this;
    api.global.engine=g;api.global.references=1;api.engine=&es;api.enter=enter;api.leave=leave;api.tick=tick;api.error=error;api.context=this;
    request.enter=enter;request.leave=leave;request.allocate=allocate;request.release=release;request.create_event=create_event;request.create_queue=queue_create;request.diagnostic=diag;request.context=this;
    slots[2]=reinterpret_cast<void*>(release_interface);slots[29]=reinterpret_cast<void*>(thread_assert);
    slots[22]=reinterpret_cast<void*>(terminate);slots[16]=reinterpret_cast<void*>(dispatch);slots[17]=reinterpret_cast<void*>(queue_close);
    gdk={query,this};scheduler.enter=enter;scheduler.leave=leave;scheduler.wait_one=wait;scheduler.context=this;
    easy={&request,&gdk,del_cs,close,this};multi={&scheduler,&request,create_event,create_multi_event,close,event,last_error,this};
}
}
int main(){
    Host h;HandleBinding b(h.api,h.easy,h.multi);
    void* raw=b.easy_init();assert(raw&&h.cs==4&&h.queues==1);
    auto* e=static_cast<P>(raw);assert(get<P>(e,0)==e&&get<P>(e,0x260)==e&&get<std::uint32_t>(e,0xac)==0x2000);
    assert(bound_easy_setopt(h.api,e,10002,reinterpret_cast<std::uintptr_t>("https://example.test/"))==0);
    void* m=b.multi_init();assert(m&&h.cs==5&&h.events==5);
    put(e,0x310,m);b.easy_cleanup(e);assert(h.cs==5&&h.errors==1);
    put<void*>(e,0x310,nullptr);put(e,0x258,2U);b.easy_cleanup(e);assert(h.errors==2&&h.cs==5);
    put(e,0x258,0U);b.easy_cleanup(e);assert(h.cs==1&&h.term==1&&h.dispatches==4&&h.queues==0);
    assert(b.multi_cleanup(m)==0&&!h.cs&&h.closed==5);
    h.fail_queue=true;assert(!b.easy_init()&&!h.cs&&h.closed==6&&h.errors==3);
    assert(b.multi_cleanup(nullptr)==1&&h.errors==4);
    assert(h.api.profile.counts[6]==2&&h.api.profile.counts[9]==3&&h.api.profile.counts[13]==1&&h.api.profile.counts[20]==2);
    assert(h.allocs==h.frees&&h.queries==h.releases&&!h.locks);
}
