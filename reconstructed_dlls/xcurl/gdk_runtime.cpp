// Original XCurl Store GDK lazy loader, initialization and QueryApiImpl wrapper.
#include "gdk_runtime.h"
using namespace t6_xcurl;
namespace {
constexpr const char* export_names[]={"QueryApiImpl","InitializeApiImpl","InitializeApiImplEx","XErrorReport","UninitializeApiImpl"};
constexpr std::uint32_t missing_runtime=0x89240101U;
constexpr std::uint32_t restricted_runtime=0x89240100U;
constexpr const char* restricted_message="XGameRuntimeInitialize must be called before making this call.";
constexpr const char* dependency_message="XGameRuntime is missing required dependencies, run GamingRepair.exe to resolve.";
bool failed(std::uint32_t hr){return (hr&0x80000000U)!=0;}
}
extern "C" std::uint32_t xcurl_research_gdk_load(GdkRuntime* r,const GdkRuntimeServices* s){
    if(!r->module)r->module=s->load("xgameruntime.dll",0x800U,s->context);
    return r->module?0U:missing_runtime;
}
extern "C" std::uint32_t xcurl_research_gdk_initialize(GdkRuntime* r,std::uint64_t flags,const GdkRuntimeServices* s){
    auto c=s->context;s->lock(c);std::uint32_t result=0;const auto restricted=static_cast<std::uint8_t>(flags&1U);
    if(!r->module)result=xcurl_research_gdk_load(r,s);
    if(!failed(result)){
        for(unsigned i=0;i<5;++i)if(!r->exports[i])r->exports[i]=s->resolve(r->module,export_names[i],c);
        auto* e=r->exports;
        if(!e[0]||(!e[1]&&!e[2])||!e[3]||!e[4])result=missing_runtime;
        else if(!e[2]&&restricted){s->report(e[3],restricted_runtime,restricted_message,c);result=restricted_runtime;}
        else if(r->initialized&&(!r->restricted||restricted))result=0;
        else{
            if(e[2])result=s->initialize(e[2],0x633600002711ULL,0x633600000c6dULL,flags|2ULL,c);
            else result=s->initialize(e[1],0x6336ULL,0x2711ULL,0,c);
            if(!failed(result)){r->initialized=1;r->restricted=restricted;}
        }
    }
    if(failed(result)&&(s->debug_flags(c)&3U)==3U)s->debug_break(c);
    s->unlock(c);return result;
}
// Original 0x5568: restricted lazy initialization does not own UninitializeApiImpl.
extern "C" void xcurl_research_gdk_shutdown(GdkRuntime* r,const GdkRuntimeServices* s){
    auto c=s->context;s->lock(c);
    if(r->initialized&&!r->restricted&&r->exports[4])s->uninitialize(r->exports[4],c);
    for(auto& e:r->exports)e=nullptr;
    r->initialized=0;r->restricted=0;
    if(r->module){s->unload(r->module,c);r->module=nullptr;}
    s->unlock(c);
}
extern "C" std::uint32_t xcurl_research_gdk_query(GdkRuntime* r,const void* cls,const void* iid,void** out,const GdkRuntimeServices* s){
    if(!r->initialized){auto status=xcurl_research_gdk_initialize(r,1,s);if(failed(status))return status;}
    auto status=s->query(r->exports[0],cls,iid,out,s->context);
    if(status==0x80040154U){status=0x89240107U;if(r->exports[3])s->report(r->exports[3],status,dependency_message,s->context);}
    return status;
}
extern "C" std::uint32_t xcurl_research_gdk_query_adapter(const void* cls,const void* iid,void** out,void* context){
    auto* adapter=static_cast<GdkQueryAdapter*>(context);
    return xcurl_research_gdk_query(adapter->runtime,cls,iid,out,adapter->services);
}
