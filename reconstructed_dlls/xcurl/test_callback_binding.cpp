#include "callback_binding.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
void circle(P p){put(p,0,p);put(p,8,p);}
struct Host {
    unsigned locks=0,signals=0,allocated=0,freed=0,queries=0,released=0;
    std::string headers,body;
    RequestServices request{};
    std::array<void*,30> slots{};
    struct Object{void** vtable;Host* h;} object{slots.data(),this};
    struct Task{void* context;void(*callback)(void*,bool);};
    std::deque<Task> queue;
    TaskQueueBinding* tasks=nullptr;
};
Host& host(void* p){return *static_cast<Host*>(p);}
Host& obj(void* p){return *static_cast<Host::Object*>(p)->h;}
void enter(void*,void* c){++host(c).locks;}
void leave(void*,void* c){assert(host(c).locks);--host(c).locks;}
void notify(void*,void* c){++host(c).signals;}
void* allocate(std::size_t n,std::uint32_t,void* c){++host(c).allocated;return std::malloc(n);}
void release(void* p,std::uint32_t,void* c){assert(p);++host(c).freed;std::free(p);}
std::uint32_t query(const void*,const void*,void** out,void* c){auto& h=host(c);++h.queries;*out=&h.object;return 0;}
std::uint32_t release_iface(void* p){++obj(p).released;return 1;}
std::uint32_t submit(void* p,void* q,std::uint32_t port,void* data,void* cb){
    auto& h=obj(p);assert(q==&h&&port==1);h.queue.push_back({data,reinterpret_cast<void(*)(void*,bool)>(cb)});return 0;
}
std::uint8_t dispatch(void* p,void* q,std::uint32_t port,std::uint32_t t){
    auto& h=obj(p);assert(q==&h&&port==1&&!t);if(h.queue.empty())return 0;
    auto task=h.queue.front();h.queue.pop_front();task.callback(task.context,false);return 1;
}
std::uint64_t writer(void* p,std::uint64_t sz,std::uint64_t n,void* c){
    assert(sz==1);static_cast<std::string*>(c)->append(static_cast<const char*>(p),n);return n;
}
std::uint32_t deliver(void*,const void* p,std::uint32_t n,void* c){host(c).body.append(static_cast<const char*>(p),n);return 0;}
}
int main(){
    Host h;GlobalLifecycle global{};
    h.request.enter=enter;h.request.leave=leave;h.request.set_event=notify;h.request.allocate=allocate;h.request.release=release;h.request.context=&h;
    h.slots[2]=reinterpret_cast<void*>(release_iface);h.slots[16]=reinterpret_cast<void*>(dispatch);h.slots[18]=reinterpret_cast<void*>(submit);
    GdkQueryServices gdk{query,&h};TaskQueueBinding tasks(global,h.request,gdk);h.tasks=&tasks;
    StartServices start{};start.request=&h.request;start.gdk=&gdk;
    StatusServices status{};status.request=&h.request;status.gdk=&gdk;
    HeaderServices headers{&h.request,nullptr,nullptr};BodyServices body{&h.request,nullptr,deliver};UploadServices upload{&h.request,nullptr,nullptr,nullptr};
    CallbackBinding callbacks(tasks,start,status,headers,body,upload),other(tasks,start,status,headers,body,upload);
    assert(callbacks.activate()&&!other.activate());
    alignas(8) std::array<std::uint8_t,0x368> easy{};auto* e=easy.data();auto* r=e+0x230;
    circle(e);put(e,0x260,e);put(e,0x350,&h);put(e,0x360,&h);
    // Exercise the actual original header worker via a queued callback.
    const char raw[]="HTTP/1.1 200 OK\r\nX: y\r\n\r\n";
    auto* header=allocate(sizeof(raw),8,&h);std::memcpy(header,raw,sizeof(raw));
    put(e,0x1d0,header);put(e,0x1c8,static_cast<std::uint64_t>(sizeof(raw)-1));put(e,0x130,&writer);put(e,0x128,&h.headers);
    assert(xcurl_research_queue_headers(e,&callbacks.headers())==0&&h.queue.size()==1);
    assert(tasks.dispatch(&h,1,0)==1&&h.headers==raw&&!get<void*>(e,0x1d0));
    // Body publication uses its embedded task context and releases the slot.
    char data[]="body data";put(e,0x340,data+0);put(e,0x338,1U);
    assert(xcurl_research_publish_body(e,data,sizeof(data)-1,&callbacks.body())==0);
    assert(tasks.dispatch(&h,1,0)==1&&h.body=="body data"&&get<std::uint32_t>(e,0x338)==0);
    // Deferred read and upload worker both use their real inactive-state guard.
    assert(xcurl_research_queue_read(e,&callbacks.body())==0&&tasks.dispatch(&h,1,0)==1);
    assert(xcurl_research_queue_upload(e,&callbacks.headers())==0&&tasks.dispatch(&h,1,0)==1);
    // The original async callback reads the request from XAsyncBlock+8.
    // State 6 makes its finish transition idempotent without needing a multi.
    put(r,0x28,6U);put(r,0x60,r);put(r,0x50,&h);
    auto start_fn=reinterpret_cast<void(*)(void*)>(CallbackBinding::start_token());start_fn(r+0x58);
    assert(get<std::uint32_t>(r,0x28)==6);
    CallbackBinding::status_entry(reinterpret_cast<std::uintptr_t>(r),0x800,nullptr,0);
    assert(!get<void*>(r,0x50)&&get<std::uint32_t>(r,0x28)==6);
    callbacks.deactivate();assert(other.activate());callbacks.deactivate();other.deactivate();
    assert(callbacks.headers().submit(&h,e,0,nullptr)==0x8000ffffU);
    assert(h.queue.empty()&&!h.locks&&h.allocated==h.freed&&h.queries==h.released&&h.signals==4);
}
