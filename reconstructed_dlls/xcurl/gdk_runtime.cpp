// Store original 0x5364, 0x5148 and 0x53A4. The OS loader and GDK exports are service boundaries.
#include "gdk_runtime.h"
using namespace t6_xcurl;
namespace {
constexpr const char* names[]={"QueryApiImpl","InitializeApiImpl","InitializeApiImplEx","XErrorReport","UninitializeApiImpl"};
constexpr std::uint32_t missing=0x89240101U;
}
extern "C" std::uint32_t xcurl_research_gdk_load(GdkRuntime* r,const GdkRuntimeServices* s){
    if(!r->module)r->module=s->load("xgameruntime.dll",0x800,s->context);
    return r->module?0:missing;
}
extern "C" std::uint32_t xcurl_research_gdk_initialize(GdkRuntime* r,std::uint64_t flags,const GdkRuntimeServices* s){
    auto c=s->context;s->lock(c);std::uint32_t result=0;auto restricted=static_cast<std::uint8_t>(flags&1);
    auto run=[&]()->std::uint32_t{
        if(auto status=xcurl_research_gdk_load(r,s))return status;
        for(unsigned i=0;i<5;++i)if(!r->exports[i])r->exports[i]=s->resolve(r->module,names[i],c);
        auto* e=r->exports;
        if(!e[0]||(!e[1]&&!e[2])||!e[3]||!e[4])return missing;
        if(!e[2]&&restricted){s->report(e[3],0x89240100U,"XGameRuntimeInitialize must be called before making this call.",c);return 0x89240100U;}
        if(r->initialized&&(!r->restricted||restricted))return 0;
        auto status=e[2]?s->initialize(e[2],0x633600002711ULL,0x633600000c6dULL,flags|2,c):s->initialize(e[1],0x6336,0x2711,0,c);
        if(!(status&0x80000000U)){r->initialized=1;r->restricted=restricted;}
        return status;
    };
    result=run();if((result&0x80000000U)&&(s->debug_flags(c)&3)==3)s->debug_break(c);s->unlock(c);return result;
}
extern "C" std::uint32_t xcurl_research_gdk_query(GdkRuntime* r,const void* cls,const void* iid,void** out,const GdkRuntimeServices* s){
    if(!r->initialized){auto status=xcurl_research_gdk_initialize(r,1,s);if(status&0x80000000U)return status;}
    auto status=s->query(r->exports[0],cls,iid,out,s->context);
    if(status==0x80040154U){status=0x89240107U;if(r->exports[3])s->report(r->exports[3],status,"XGameRuntime is missing required dependencies, run GamingRepair.exe to resolve.",s->context);}
    return status;
}
