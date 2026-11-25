#include "transfer_binding.h"
#include "task_queue_binding.h"
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
    void (*callback)(void*,bool)=nullptr;
    TaskQueueBinding* tasks=nullptr;
    unsigned creates=0,cancels=0;
    bool fail_submit=false,fail_message=false;
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
void* alloc(std::size_t n,std::uint32_t tag,void* c){assert(tag==12||tag==7);if(tag==12){if(host(c).fail_message)return nullptr;++host(c).allocs;}return std::malloc(n);}
void release(void* p,std::uint32_t tag,void* c){assert(tag==12||tag==7);if(tag==12)++host(c).frees;std::free(p);}
void adjust(std::uint32_t n,void* c){host(c).tasks->adjust_global(n);}
void diag(void*,std::uint32_t site,std::uint32_t status,void*){assert((site==0x20f80||site==0x20920)&&status==0x80004005U);}
std::uint32_t submit(void* q,void* e,void* c){return host(c).tasks->submit_completion(q,e);}
std::uint8_t dispatch(void* q,std::uint32_t p,std::uint32_t t,void* c){return host(c).tasks->dispatch(q,p,t);}
std::uint32_t gdk_submit(void* o,void* queue,std::uint32_t port,void* context,void* cb){
    auto& h=obj(o);assert(queue==h.queue&&port==1&&!h.pending);if(h.fail_submit)return 0x80004005U;
    h.pending=context;h.callback=reinterpret_cast<void(*)(void*,bool)>(cb);++h.submits;return 0;
}
std::uint8_t gdk_dispatch(void* o,void* queue,std::uint32_t port,std::uint32_t timeout){
    auto& h=obj(o);assert(queue==h.queue&&port==1&&!timeout);if(!h.pending)return 0;
    auto* p=h.pending;h.pending=nullptr;h.callback(p,h.submits==2);return 1;
}
std::uint32_t gdk_create(void* o,std::uint32_t a,std::uint32_t b,void** out){auto& h=obj(o);assert(a==0&&b==0);++h.creates;*out=h.queue;return 0;}
void gdk_cancel(void* o,void* p){auto& h=obj(o);assert(p==h.queue);++h.cancels;}
void thread_assert(void*){}
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
    h.vtable[12]=reinterpret_cast<void*>(gdk_create);h.vtable[16]=reinterpret_cast<void*>(gdk_dispatch);
    h.vtable[18]=reinterpret_cast<void*>(gdk_submit);h.vtable[5]=reinterpret_cast<void*>(gdk_cancel);h.vtable[29]=reinterpret_cast<void*>(thread_assert);
    GdkQueryServices gdk{query,&h};TaskQueueBinding tasks(h.global,h.request,gdk);h.tasks=&tasks;
    void* q=nullptr;assert(tasks.create(&q)==0&&q==h.queue&&h.creates==1);tasks.cancel(q);assert(h.cancels==1);
    h.fail_submit=true;assert(tasks.submit_completion(q,nullptr)==0x80004005U&&!h.pending);h.fail_submit=false;
    PrepareServices prepare{};prepare.request=&h.request;prepare.gdk=&gdk;
    prepare.prepare_url=[](void*,void*,void* out,void* c)->std::uint32_t{auto* q=alloc(8,7,c);std::memcpy(q,u"url",8);std::memcpy(out,&q,8);return 0;};
    prepare.copy_url=[](const void* src,void* out,void* c)->std::uint32_t{auto* q=alloc(8,7,c);std::memcpy(q,src,8);std::memcpy(out,&q,8);return 0;};
    for(auto* e:{e1.data(),e2.data(),e3.data()})put(e,0x88,"url");
    MultiHostServices os{wait_one,wait_many,unlock,reset,&h};TransferBinding b(h.global,prepare,os);
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
    // Force the recovered BA8C synchronous fallback during admission.
    tasks.enable_reservations();bool created=false;
    h.fail_message=true;assert(!tasks.reserve(pending,created)&&!created&&tasks.reservations_empty());
    h.fail_message=false;assert(tasks.reserve(pending,created)&&created);
    assert(tasks.reserve(pending,created)&&!created);
    h.fail_submit=true;assert(b.add(m,pending)==0);
    h.fail_message=true; // force allocation failure throughout completion
    assert(b.perform(m,&running)==0&&running==0&&h.starts==3);
    assert(get<std::uint32_t>(g,0x40)==0&&get<std::uint32_t>(m,0x3c)==0);
    msg=static_cast<P>(b.info_read(m,&remaining));
    assert(msg&&remaining==0&&get<void*>(msg,8)==pending&&get<std::uint32_t>(msg,16)==58);
    assert(!b.info_read(m,&remaining)&&h.allocs==3&&h.frees==3&&tasks.reservations_empty());
    assert(b.remove(m,pending)==0&&get<std::uint32_t>(m,0x38)==0);
    h.fail_submit=false;h.fail_message=false;
    assert(tasks.reserve(pending,created)&&created);tasks.discard(pending);assert(tasks.reservations_empty()&&h.allocs==4&&h.frees==4);
    std::uint32_t count=99;
    assert(b.poll(m,42,&count)==0&&count==0);
    assert(b.wait(m,1,&count)==0&&count==1);
    assert(b.wakeup(m)==0);
    assert(h.locks==0&&h.resets==3&&h.releases>=2&&!h.pending);
    for(auto* e:{e1.data(),e2.data(),e3.data()})xcurl_research_easy_reset_transfer(e,&h.request);
    h.global.engine=nullptr;assert(b.perform(m,&running)==1);
}
