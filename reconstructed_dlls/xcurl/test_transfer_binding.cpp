#include "transfer_binding.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=unsigned char*;
template<class T>void put(P p,unsigned off,T v){std::memcpy(p+off,&v,sizeof(v));}
template<class T>T get(P p,unsigned off){T v;std::memcpy(&v,p+off,sizeof(v));return v;}
void circle(P p){put(p,0,p);put(p,8,p);}
struct Host{
    GlobalLifecycle global;
    RequestServices request{};
    unsigned locks=0,signals=0,resets=0,waits=0,unlocks=0,starts=0,submits=0,allocs=0,frees=0,releases=0;
    void* pending=nullptr;
    void* queue=nullptr;
    std::array<void*,30> vtable{};
    struct Object{void** table;Host* host;} object{vtable.data(),this};
};
Host& host(void* p){return *static_cast<Host*>(p);}
Host& obj(void* p){return *static_cast<Host::Object*>(p)->host;}
void enter(void*,void* c){++host(c).locks;}
void leave(void*,void* c){assert(host(c).locks);--host(c).locks;}
void signal(void*,void* c){++host(c).signals;}
void reset(void*,void* c){++host(c).resets;}
void unlock(void*,void* c){++host(c).unlocks;}
std::uint32_t wait_one(void*,std::uint32_t t,void* c){++host(c).waits;return t==42?0x102:0;}
std::uint32_t wait_many(std::uint32_t n,const void*,std::uint32_t t,void* c){assert(n==2);return wait_one(nullptr,t,c);}
void* alloc(std::size_t n,std::uint32_t tag,void* c){assert(tag==12);++host(c).allocs;return std::malloc(n);}
void release(void* p,std::uint32_t tag,void* c){assert(tag==12);++host(c).frees;std::free(p);}
void adjust(std::uint32_t n,void* c){auto& h=host(c);xcurl_research_engine_decrement(h.global.engine,n,&h.request);}
void diag(void*,std::uint32_t site,std::uint32_t status,void*){assert(site==0x20f80&&status==0x80004005U);}
std::uint32_t submit(void* queue,void* easy,void* c){auto& h=host(c);assert(queue==h.queue&&!h.pending);h.pending=easy;++h.submits;return 0;}
std::uint8_t dispatch(void* queue,std::uint32_t port,std::uint32_t timeout,void* c){
    auto& h=host(c);assert(queue==h.queue&&port==1&&!timeout);if(!h.pending)return 0;
    auto* p=h.pending;h.pending=nullptr;xcurl_research_easy_complete(p,&h.request);return 1;
}
void after_cancel(void*,void*){assert(false);}
std::uint32_t query(const void*,const void*,void** out,void* c){*out=&host(c).object;return 0;}
std::uint32_t release_interface(void* p){++obj(p).releases;return 1;}
std::uint32_t security_async(void* p,const char16_t*,void* block){
    auto& h=obj(p);assert(get<void*>(static_cast<P>(block),0)==h.queue);
    assert(get<void*>(static_cast<P>(block),8)==static_cast<P>(block)-0x58);
    ++h.starts;return 0x80004005U;
}
}
int main(){
    Host h;
    alignas(8) std::array<unsigned char,0xa8> engine{};
    alignas(8) std::array<unsigned char,0xc8> multi{};
    alignas(8) std::array<unsigned char,0x368> e1{},e2{},e3{};
    auto* g=engine.data();auto* m=multi.data();h.queue=&h;h.global.engine=g;
    put(g,0x44,1U);g[0x49]=1;put(g,0x90,h.queue);
    for(auto o:{0x40U,0x50U,0x60U,0x78U})circle(m+o);
    for(auto o:{0xa0U,0xa8U,0xb0U,0xb8U})put(m,o,h.queue);
    for(auto* e:{e1.data(),e2.data(),e3.data()}){circle(e);put(e,0x260,e);put(e,0x350,h.queue);put(e,0x360,h.queue);}
    h.request.enter=enter;h.request.leave=leave;h.request.set_event=signal;h.request.allocate=alloc;
    h.request.release=release;h.request.adjust_global=adjust;h.request.diagnostic=diag;h.request.submit_completion=submit;
    h.request.queue_dispatch=dispatch;h.request.context=&h;
    h.vtable[2]=reinterpret_cast<void*>(release_interface);h.vtable[11]=reinterpret_cast<void*>(security_async);
    GdkQueryServices gdk{query,&h};PrepareServices prepare{};prepare.request=&h.request;prepare.gdk=&gdk;
    MultiHostServices os{wait_one,wait_many,unlock,reset,after_cancel,&h};TransferBinding b(h.global,prepare,os);
    auto* a=e1.data();auto* z=e2.data();auto* pending=e3.data();
    assert(b.add(nullptr,a)==1&&b.add(m,nullptr)==2);
    assert(b.add(m,a)==0&&b.add(m,a)==7&&b.add(m,z)==0&&b.add(m,pending)==0);
    assert(b.remove(m,pending)==0&&get<void*>(pending,0x310)==nullptr);
    std::uint32_t running=99;
    assert(b.perform(m,&running)==0&&running==1&&h.starts==1&&h.submits==1);
    assert(get<std::uint32_t>(g,0x40)==0&&get<std::uint32_t>(m,0x3c)==0);
    assert(b.perform(m,&running)==0&&running==0&&h.starts==2&&h.submits==2);
    assert(get<std::uint32_t>(g,0x40)==0&&get<std::uint32_t>(m,0x88)==2);
    std::uint32_t remaining=99;auto* msg=static_cast<P>(b.info_read(m,&remaining));
    assert(msg&&remaining==1&&get<std::uint32_t>(msg,0)==1&&get<void*>(msg,8)==z&&get<std::uint32_t>(msg,16)==58);
    msg=static_cast<P>(b.info_read(m,&remaining));
    assert(msg&&remaining==0&&get<void*>(msg,8)==a&&get<std::uint32_t>(msg,16)==58);
    assert(!b.info_read(m,&remaining)&&remaining==0&&h.allocs==2&&h.frees==2);
    assert(b.remove(m,a)==0&&b.remove(m,z)==0&&get<std::uint32_t>(m,0x38)==0);
    std::uint32_t count=99;
    assert(b.poll(m,42,&count)==0&&count==0);
    assert(b.wait(m,1,&count)==0&&count==1);
    assert(b.wakeup(m)==0);
    assert(h.locks==0&&h.resets==2&&h.releases==2&&!h.pending);
    h.global.engine=nullptr;assert(b.perform(m,&running)==1);
}
