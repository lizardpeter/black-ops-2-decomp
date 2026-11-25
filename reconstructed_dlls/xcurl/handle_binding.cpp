#include "handle_binding.h"
#include <cstring>
namespace t6_xcurl {
HandleBinding::HandleBinding(ApiBinding& a,const EasyLifecycleServices& e,const MultiLifecycleServices& m)
    :api_(a),easy_(e),multi_(m),handles_{allocate,release,initialize_lock,start_global,start_easy,destroy_easy,start_multi,destroy_multi,this}{}
HandleBinding& HandleBinding::self(void* c){return *static_cast<HandleBinding*>(c);}
void HandleBinding::record(std::uint32_t id){xcurl_research_record_api(&api_.profile,id,api_.profile_lock,api_.enter,api_.leave,api_.tick,api_.context);}
void HandleBinding::error(std::uint32_t code,std::uint32_t id){if(code&&api_.error)api_.error(code,id,api_.context);}
void* HandleBinding::allocate(std::size_t n,std::uint32_t t,void* c){auto* s=self(c).api_.engine;return s->allocate(n,t,s->context);}
void HandleBinding::release(void* p,std::uint32_t t,void* c){auto* s=self(c).api_.engine;s->release(p,t,s->context);}
void HandleBinding::initialize_lock(void* p,void* c){auto* s=self(c).api_.engine;s->initialize_cs(p,s->context);}
std::uint32_t HandleBinding::start_global(void* c){
    auto& a=self(c).api_;return bound_engine_bootstrap(&a.global,a.engine_limit,&a.frequency,a.engine,&a.startup);
}
std::uint32_t HandleBinding::start_easy(void* p,void* c){return xcurl_research_easy_start_engine(p,self(c).easy_.request);}
void HandleBinding::destroy_easy(void* p,void* c){xcurl_research_easy_destroy_engine(p,&self(c).easy_);}
std::uint32_t HandleBinding::start_multi(void* p,void* c){
    auto& b=self(c);auto status=xcurl_research_multi_start_engine(p,b.api_.global.engine,&b.multi_);
    // Original 18110 closes handles but does not delete the critical section.
    // Balance the actual native resource before the wrapper frees storage.
    if(status)b.api_.engine->delete_cs(static_cast<std::uint8_t*>(p)+0x10,b.api_.engine->context);
    return status;
}
std::uint32_t HandleBinding::destroy_multi(void* p,void* c){
    auto& b=self(c);auto status=xcurl_research_multi_destroy_engine(p,b.api_.global.engine,&b.multi_);
    b.api_.engine->delete_cs(static_cast<std::uint8_t*>(p)+0x10,b.api_.engine->context);return status;
}
void* HandleBinding::easy_init(){
    record(6);void* p=nullptr;
    auto status=api_.global.references&&!api_.global.engine?2U:xcurl_research_easy_create(&p,&api_.global.references,&handles_);
    error(status,6);return status?nullptr:p;
}
void HandleBinding::easy_cleanup(void* p){(void)try_easy_cleanup(p);}
bool HandleBinding::try_easy_cleanup(void* p){
    record(9);if(p){
        auto* e=static_cast<std::uint8_t*>(p);auto* r=easy_.request;void* owner=nullptr;
        r->enter(e+0x10,r->context);std::memcpy(&owner,e+0x310,sizeof(owner));r->leave(e+0x10,r->context);
        // Avoid freeing an object still linked into a live multi or still
        // receiving asynchronous notifications. Original does not enforce this.
        r->enter(e+0x230,r->context);std::uint32_t state=0;std::memcpy(&state,e+0x258,sizeof(state));r->leave(e+0x230,r->context);
        if(owner||state){error(2,9);return false;}
        xcurl_research_easy_destroy(p,&handles_);
    }
    return true;
}
void* HandleBinding::multi_init(){
    record(13);void* p=nullptr;
    auto status=api_.global.engine?xcurl_research_multi_create(&p,&handles_):4U;
    error(status,13);return status?nullptr:p;
}
std::uint32_t HandleBinding::multi_cleanup(void* p){
    record(20);auto status=api_.global.engine?xcurl_research_multi_destroy(p,&handles_):1U;
    error(status,20);return status;
}
}
