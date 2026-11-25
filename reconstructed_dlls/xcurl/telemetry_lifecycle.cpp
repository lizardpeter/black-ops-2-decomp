// Original object stores 0x7A30, allocation 0x7DF4, record unlink/free
// 0x7C50/0x8018 and record destructor 0x7CF8. The no-publisher integration
// intentionally does not reproduce provider registration, event flush or
// global provider state. The GUID formatter uses the original byte table
// at RVA 0x23530. Evidence 45551835362 and 45551949569.
#include "telemetry_lifecycle.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
}
extern "C" void* xcurl_research_telemetry_construct(void* p,const TelemetryServices* s){
    auto* b=static_cast<P>(p);std::memset(b,0,0x28);
    put(b,0x28,b+0x28);put(b,0x30,b+0x28);
    put(b,0x38,static_cast<std::uint64_t>(s->tick32(s->context)));return p;
}
extern "C" std::uint32_t xcurl_research_telemetry_create(void** out,const TelemetryServices* s){
    *out=s->allocate(telemetry_bytes,3,s->context);
    if(!*out)return 0x8007000eU;
    xcurl_research_telemetry_construct(*out,s);return 0;
}
extern "C" void xcurl_research_telemetry_unlink(void* p){
    auto* n=static_cast<P>(p);auto* next=get<P>(n,0);auto* prev=get<P>(n,8);
    put(next,8,prev);put(prev,0,next);put(n,0,n);put(n,8,n);
}
extern "C" void xcurl_research_telemetry_destroy_records(void* p,const TelemetryServices* s){
    auto* h=static_cast<P>(p)+0x28;
    for(;;){auto* n=get<P>(h,0);if(!n||n==h)break;
        xcurl_research_telemetry_unlink(n);s->release(n,2,s->context);
    }
}
extern "C" void xcurl_research_telemetry_destroy_local(void* p,const TelemetryServices* s){
    if(p){xcurl_research_telemetry_destroy_records(p,s);s->release(p,3,s->context);}
}
extern "C" void xcurl_research_telemetry_format_guid(const std::uint8_t* g,char16_t* out){
    constexpr unsigned order[]={3,2,1,0,45,5,4,45,7,6,45,8,9,45,10,11,12,13,14,15};
    constexpr char16_t hex[]=u"0123456789abcdef";
    for(auto i:order){if(i==45)*out++=u'-';else{*out++=hex[g[i]>>4];*out++=hex[g[i]&15];}}
    *out=0;
}
