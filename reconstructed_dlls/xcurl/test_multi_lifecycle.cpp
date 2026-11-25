#include "multi_lifecycle.h"
#include "handle_lifecycle.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
void circle(P p){put(p,0,p);put(p,8,p);}
void append(P h,P n){put(n,0,h);put(n,8,get<P>(h,8));put(get<P>(h,8),0,n);put(h,8,n);}
struct Host{
    alignas(8) std::array<std::uint8_t,0xa8> engine{};
    RequestServices request{};
    SchedulerServices scheduler{};
    MultiLifecycleServices life{};
    unsigned lock=0,created=0,closed=0,allocs=0,frees=0,waits=0,cancels=0,progress=0,signals=0,fail_at=0;
    std::array<unsigned,5> handles{};
    void* idle=nullptr;
    bool idle_signaled=true;
    Host();
};
Host& host(void* c){return *static_cast<Host*>(c);}
void enter(void*,void* c){++host(c).lock;}
void leave(void*,void* c){assert(host(c).lock);--host(c).lock;}
void* allocate(std::size_t n,std::uint32_t,void* c){++host(c).allocs;return std::malloc(n);}
void release(void* p,std::uint32_t,void* c){++host(c).frees;std::free(p);}
void* create(void* c){auto& h=host(c);if(++h.created==h.fail_at)return nullptr;return &h.handles[h.created];}
void* event(std::uint8_t m,std::uint8_t i,void* c){auto& h=host(c);assert(m==i&&m==(h.created==3));auto* p=create(c);if(m)h.idle=p;return p;}
void close(void* p,void* c){assert(p);++host(c).closed;}
std::uint32_t error(void*){return 8;}
std::uint32_t wait(void* p,std::uint32_t t,void* c){auto& h=host(c);++h.waits;return p==h.idle&&t==0&&!h.idle_signaled?0x102:0;}
void unlock(void*,void*){}
void signal(void* p,void* c){auto& h=host(c);++h.signals;if(p==h.idle)h.idle_signaled=true;}
void cancel(void* e,std::uint32_t status,std::uint32_t code,void* c){auto& h=host(c);++h.cancels;xcurl_research_easy_cancel(e,status,code,&h.request);}
void stop(void* e,void* c){xcurl_research_easy_stop(e,&host(c).request);}
void progress(void* e,void* c){auto& h=host(c);++h.progress;xcurl_research_easy_complete(e,&h.request);}
void adjust(std::uint32_t n,void* c){auto& h=host(c);xcurl_research_engine_decrement(h.engine.data(),n,&h.request);}
Host::Host(){
    circle(engine.data()+0x30);put(engine.data(),0x28,&handles[0]);
    request.enter=enter;request.leave=leave;request.allocate=allocate;request.release=release;request.set_event=signal;request.adjust_global=adjust;request.context=this;
    scheduler.enter=enter;scheduler.leave=leave;scheduler.cancel_easy=cancel;scheduler.after_cancel=stop;scheduler.progress_easy=::progress;scheduler.wait_one=wait;scheduler.context=this;
    life={&scheduler,&request,create,event,close,unlock,error,this};
}
std::uint32_t start(void* m,void* c){auto& h=host(c);return xcurl_research_multi_start_engine(m,h.engine.data(),&h.life);}
std::uint32_t destroy(void* m,void* c){auto& h=host(c);return xcurl_research_multi_destroy_engine(m,h.engine.data(),&h.life);}
HandleServices services(Host& h){HandleServices s{};s.allocate=allocate;s.release=release;s.initialize_lock=[](void*,void*){};s.start_multi=start;s.destroy_multi=destroy;s.context=&h;return s;}
void init_easy(P e,P m){circle(e);put(e,0x310,m);put(e,0x260,e);}
}
int main(){
    {
        Host h;auto s=services(h);void* raw=nullptr;
        assert(xcurl_research_multi_create(&raw,&s)==0&&raw&&h.created==4);
        auto* m=static_cast<P>(raw);assert(get<P>(h.engine.data(),0x30)==m);
        alignas(8) std::array<std::uint8_t,0x368> pending{},active{},done{};
        init_easy(pending.data(),m);init_easy(active.data(),m);init_easy(done.data(),m);
        append(m+0x40,pending.data());append(m+0x50,active.data());append(m+0x60,done.data());
        put(m,0x3c,1U);put(h.engine.data(),0x40,1U);h.idle_signaled=false;
        assert(xcurl_research_multi_destroy(m,&s)==0);
        assert(h.cancels==1&&h.progress==1&&h.signals==1);
        assert(get<std::uint32_t>(active.data(),0x220)==42&&get<std::uint32_t>(active.data(),0x228)==0x80004004U);
        for(auto* p:{pending.data(),active.data(),done.data()})assert(!get<void*>(p,0x310)&&get<P>(p,0)==p);
        assert(get<P>(h.engine.data(),0x30)==h.engine.data()+0x30&&get<std::uint32_t>(h.engine.data(),0x40)==0);
        assert(h.closed==4&&h.allocs==h.frees&&h.lock==0);
    }
    for(unsigned fail=1;fail<=4;++fail){
        Host h;h.fail_at=fail;auto s=services(h);void* m=nullptr;
        assert(xcurl_research_multi_create(&m,&s)==4&&!m);
        assert(h.created==fail&&h.closed==fail-1&&h.allocs==h.frees&&!h.lock);
        assert(get<P>(h.engine.data(),0x30)==h.engine.data()+0x30);
    }
    {
        Host h;h.engine[0x48]=1;auto s=services(h);void* m=nullptr;
        assert(xcurl_research_multi_create(&m,&s)==0&&static_cast<P>(m)[0xc0]==1);
        assert(xcurl_research_multi_destroy(m,&s)==0&&h.closed==4&&h.allocs==h.frees);
    }
}
