#include "api_binding.h"
#include "handle_lifecycle.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace t6_xcurl;
namespace {
struct Host {
    unsigned allocs=0,frees=0,starts=0,shutdowns=0,errors=0,active=0;
    unsigned cs_init=0,cs_delete=0,locks=0,telemetry=0,queue=0,mutex=0,app=0,registered=0;
    unsigned queries=0,releases=0;
    bool fail_alloc=false,fail_runtime=false,fail_register=false;
    std::array<void*,30> slots{};
    struct Object {void** table;Host* host;} object{slots.data(),this};
};
Host& host(void* p){return *static_cast<Host*>(p);}
Host& obj(void* p){return *static_cast<Host::Object*>(p)->host;}
void enter(void*,void* c){++host(c).locks;}
void leave(void*,void* c){assert(host(c).locks);--host(c).locks;}
void init_cs(void*,void* c){++host(c).cs_init;}
void delete_cs(void*,void* c){++host(c).cs_delete;}
std::uint64_t tick(void*){return 42;}
void error(std::uint32_t,std::uint32_t,void* c){++host(c).errors;}
void active(void*,void* c){++host(c).active;}
void* allocate(std::size_t n,std::uint32_t,void* c){if(host(c).fail_alloc)return nullptr;++host(c).allocs;return std::malloc(n);}
void release(void* p,std::uint32_t,void* c){assert(p);++host(c).frees;std::free(p);}
std::uint32_t start(void* c){++host(c).starts;return host(c).fail_runtime?0x80004005U:0;}
void shutdown(void* c){++host(c).shutdowns;}
std::uint32_t frequency(std::uint64_t* out,void*){*out=10000000;return 1;}
std::uint32_t telemetry_create(void** out,void* c){++host(c).telemetry;*out=c;return 0;}
void telemetry_destroy(void* p,void* c){assert(p==c&&host(c).telemetry==1);--host(c).telemetry;}
void* create_mutex(void* c){++host(c).mutex;return c;}
void close_handle(void* p,void* c){assert(p==c&&host(c).mutex==1);--host(c).mutex;}
std::uint32_t register_app(void*,void*,void** out,void* c){++host(c).app;*out=c;return 0;}
void unregister_app(void* p,void* c){assert(p==c&&host(c).app==1);--host(c).app;}
std::uint32_t query(const void*,const void*,void** out,void* c){++host(c).queries;*out=&host(c).object;return 0;}
std::uint32_t release_interface(void* p){++obj(p).releases;return 1;}
void assert_thread(void*){}
std::uint32_t queue_create(void* p,std::uint32_t a,std::uint32_t b,void** out){assert(a==1&&b==1);auto& h=obj(p);++h.queue;*out=&h;return 0;}
std::uint32_t queue_terminate(void* p,void* q,std::uint8_t wait,void*,void*){assert(q==&obj(p)&&wait==1);return 0;}
void queue_close(void* p,void* q){assert(q==&obj(p)&&obj(p).queue==1);--obj(p).queue;}
std::uint32_t connectivity_register(void* p,void* q,void* engine,void*,void* token){
    auto& h=obj(p);assert(q==&h&&engine);
    if(h.fail_register)return 0x80004005U;
    ++h.registered;const std::uint64_t v=42;std::memcpy(token,&v,sizeof(v));return 0;
}
std::uint8_t connectivity_unregister(void* p,std::uint64_t token,std::uint8_t wait){
    auto& h=obj(p);assert(wait==1);if(token){assert(token==42&&h.registered==1);--h.registered;}return 1;
}
template<class F> void slot(Host& h,unsigned i,F fn){h.slots[i]=reinterpret_cast<void*>(fn);}
struct Fixture {
    Host h;
    GdkQueryServices gdk{query,&h};
    SessionServices sessions{};
    EngineServices engine{};
    ApiBinding b{};
    Fixture(){
        slot(h,2,release_interface);slot(h,12,queue_create);slot(h,17,queue_close);
        // The same numeric slot is used by different interfaces. Select
        // the networking vtable in the query callback below.
        slot(h,22,queue_terminate);slot(h,29,assert_thread);
        gdk.query=[](const void* cls,const void*,void** out,void* c)->std::uint32_t{
            std::uint32_t id;std::memcpy(&id,cls,4);auto& h=host(c);
            if(id==0x37e56907U){slot(h,16,connectivity_register);slot(h,17,connectivity_unregister);}
            else slot(h,17,queue_close);
            return query(cls,nullptr,out,c);
        };
        sessions.enter=enter;sessions.leave=leave;sessions.context=&h;
        engine.gdk=&gdk;engine.sessions=&sessions;engine.initialize_cs=init_cs;engine.delete_cs=delete_cs;
        engine.enter=enter;engine.leave=leave;engine.create_mutex=create_mutex;engine.close_handle=close_handle;
        engine.telemetry_create=telemetry_create;engine.telemetry_destroy=telemetry_destroy;
        engine.register_app_state=register_app;engine.unregister_app_state=unregister_app;
        engine.allocate=allocate;engine.release=release;engine.initialize_runtime=start;engine.query_frequency=frequency;engine.context=&h;
        b.engine=&engine;b.engine_limit=77;b.profile_lock=&h;b.enter=enter;b.leave=leave;b.tick=tick;
        b.error=error;b.active_log=active;b.shutdown_runtime=shutdown;b.context=&h;
    }
    void balanced(){assert(h.allocs==h.frees&&h.cs_init==h.cs_delete&&h.queries==h.releases);assert(!h.locks&&!h.telemetry&&!h.queue&&!h.mutex&&!h.app&&!h.registered);}
};
template<class T>T read(const void* p,std::size_t n){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+n,sizeof(v));return v;}
}
int main(){
    {
        Fixture f;auto& b=f.b;auto& h=f.h;
        assert(bound_global_init(b)==0&&bound_global_init(b)==0);
        assert(b.global.engine&&b.global.references==2&&h.starts==1&&b.frequency==10000000);
        assert(read<std::uint32_t>(b.global.engine,0x44)==77);
        assert(b.startup.stage==BootstrapStage::ready&&!b.startup.status&&b.startup.frequency_available);
        assert(h.queue==1&&h.telemetry==1&&h.mutex==1&&h.app==1&&h.registered==1);
        alignas(8) std::array<unsigned char,0x368> easy{};
        char url[]="https://example.test/one";
        assert(bound_easy_setopt(b,easy.data(),10002,reinterpret_cast<std::uintptr_t>(url))==0);
        url[0]='X';auto* copy=read<const char*>(easy.data(),0x88);assert(std::string(copy)=="https://example.test/one");
        assert(bound_easy_setopt(b,easy.data(),10018,reinterpret_cast<std::uintptr_t>("agent"))==0);
        assert(std::u16string(read<const char16_t*>(easy.data(),0xd8))==u"agent");
        assert(bound_easy_setopt(b,easy.data(),98,1)==0&&read<std::uint32_t>(easy.data(),0xac)==0x2000);
        assert(bound_easy_setopt(b,nullptr,98,1)==0x2b&&h.errors==1);
        std::uintptr_t busy=1;std::memcpy(easy.data()+0x310,&busy,8);
        assert(bound_easy_setopt(b,easy.data(),98,0x9000)==2&&h.active==1&&h.errors==2);
        assert(read<std::uint32_t>(easy.data(),0xac)==0x2000);
        std::uint32_t code=204,out=0;std::memcpy(easy.data()+0x1e0,&code,4);
        assert(bound_easy_getinfo(b,easy.data(),0x200002,&out)==0&&out==204);
        xcurl_research_options_destroy(easy.data()+0x78,release,&h);
        bound_global_cleanup(b);assert(b.global.engine&&b.global.references==1&&h.shutdowns==0);
        bound_global_cleanup(b);bound_global_cleanup(b);
        assert(!b.global.engine&&!b.global.references&&!b.frequency&&h.shutdowns==1);
        assert(b.profile.counts[0]==2&&b.profile.counts[3]==3&&b.profile.counts[7]==5&&b.profile.counts[10]==1);
        f.balanced();
    }
    for(unsigned mode=0;mode<3;++mode){
        Fixture f;auto& b=f.b;auto& h=f.h;
        h.fail_alloc=mode==0;h.fail_runtime=mode==1;h.fail_register=mode==2;
        assert(bound_global_init(b)==2&&bound_global_init(b)==2);
        assert(!b.global.engine&&b.global.references==2&&h.errors==2&&h.starts==1);
        assert(b.startup.stage==(mode==1?BootstrapStage::runtime:BootstrapStage::engine));
        assert(b.startup.status==(mode==0?27U:0x80004005U)&&!b.startup.frequency_available);
        bound_global_cleanup(b);assert(h.shutdowns==0);
        bound_global_cleanup(b);assert(h.shutdowns==1&&!b.global.references);
        f.balanced();
        h.fail_alloc=h.fail_runtime=h.fail_register=false;
        assert(bound_global_init(b)==0&&b.global.engine&&h.starts==2);
        assert(b.startup.stage==BootstrapStage::ready&&b.startup.frequency_available);
        bound_global_cleanup(b);assert(h.shutdowns==2);f.balanced();
    }
}
