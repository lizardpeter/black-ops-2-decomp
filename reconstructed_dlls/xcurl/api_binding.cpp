// Compose previously machine-checked public wrappers with their recovered
// callees. Only host services remain injectable here, not the engine or
// option-dispatch algorithms themselves.
#include "api_binding.h"
#include "wide_string.h"
namespace t6_xcurl {
namespace {
ApiBinding& binding(void* c){return *static_cast<ApiBinding*>(c);}
void enter(void* p,void* c){auto& b=binding(c);b.enter(p,b.context);}
void leave(void* p,void* c){auto& b=binding(c);b.leave(p,b.context);}
std::uint64_t tick(void* c){auto& b=binding(c);return b.tick(b.context);}
void error(std::uint32_t status,std::uint32_t api,void* c){auto& b=binding(c);if(b.error)b.error(status,api,b.context);}
void active(void* p,void* c){auto& b=binding(c);if(b.active_log)b.active_log(p,b.context);}
std::uint32_t init(GlobalLifecycle* global,void* c){
    auto& b=binding(c);
    return xcurl_research_engine_bootstrap(global,b.engine_limit,&b.frequency,b.engine);
}
void destroy(void* p,void* c){
    // Bootstrap may have failed while the original refcount stayed nonzero.
    // Preserve reference accounting, but never destroy a null engine.
    if(p)xcurl_research_engine_destroy(p,binding(c).engine);
}
void release(void* p,std::uint32_t tag,void* c){
    if(p){auto* s=binding(c).engine;s->release(p,tag,s->context);}
}
void shutdown(void* c){auto& b=binding(c);if(b.shutdown_runtime)b.shutdown_runtime(b.context);b.frequency=0;}
std::uint32_t dispatch(void* p,int option,const std::uint64_t* arg,void* c){
    auto* s=binding(c).engine;
    return xcurl_research_dispatch_owned_option(p,option,arg,s->allocate,s->release,s->context);
}
}
std::uint32_t bound_global_init(ApiBinding& b){
    auto status=xcurl_research_global_init(&b.global,&b.profile,b.profile_lock,enter,leave,tick,init,error,&b);
    // The faithful wrapper returns success for a repeated init after a
    // failed first init. The source-link integration must not advertise a
    // usable engine in that state. Balance all references before retrying.
    if(!status&&!b.global.engine){status=2;error(status,0,&b);}
    return status;
}
void bound_global_cleanup(ApiBinding& b){
    xcurl_research_global_cleanup(&b.global,&b.profile,b.profile_lock,enter,leave,tick,destroy,release,shutdown,&b);
}
std::uint32_t bound_easy_setopt(ApiBinding& b,void* easy,int option,std::uint64_t arg){
    return xcurl_research_easy_setopt(easy,option,arg,&b.profile,b.profile_lock,enter,leave,tick,dispatch,active,error,&b);
}
std::uint32_t bound_easy_getinfo(ApiBinding& b,const void* easy,int option,void* out){
    return xcurl_research_easy_getinfo(easy,option,out,&b.profile,b.profile_lock,enter,leave,tick,error,&b);
}
}
