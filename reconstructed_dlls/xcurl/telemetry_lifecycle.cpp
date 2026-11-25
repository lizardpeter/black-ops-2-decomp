// Original object stores 0x7A30, allocation 0x7DF4, record unlink/free
// 0x7C50/0x8018 and record destructor 0x7CF8. The no-publisher integration
// intentionally does not reproduce provider registration, event flush or
// global provider state. The GUID formatter uses the original byte table
// at RVA 0x23530. Evidence 45551835362 and 45551949569.
#include "telemetry_lifecycle.h"
#include <cstring>
#include <array>
#include <charconv>
#include <algorithm>
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
// 8C50 uses a 500-code-unit buffer and 20-record batches. Formatting
// 205B8 is L"%i,%lu,%llu;" (Win32 long is 32 bits). The original 8970
// leaves a truncated, terminated prefix on insufficient capacity and
// flushes that prefix before retiring the overflowing record.
extern "C" void xcurl_research_telemetry_flush(void* p,const TelemetryFlushServices* s){
    if(!p)return;
    auto* lifetime=s->lifetime;auto* head=static_cast<P>(p)+0x28;
    std::array<char16_t,500> batch{};std::size_t used=0;unsigned count=0;
    for(auto* n=get<P>(head,0);n&&n!=head;n=get<P>(head,0)){
        std::array<char,64> record{};auto* cur=record.data();auto* end=record.data()+record.size();
        cur=std::to_chars(cur,end,get<std::int32_t>(n,0x10)).ptr;*cur++=',';
        cur=std::to_chars(cur,end,get<std::uint32_t>(n,0x14)).ptr;*cur++=',';
        cur=std::to_chars(cur,end,get<std::uint64_t>(n,0x18)).ptr;*cur++=';';
        auto length=static_cast<std::size_t>(cur-record.data());
        auto copied=std::min(length,batch.size()-used-1);
        for(std::size_t i=0;i<copied;++i)batch[used+i]=static_cast<unsigned char>(record[i]);
        used+=copied;batch[used]=0;
        bool overflow=copied!=length;
        if(overflow)count=0;else count=(count+1)%20;
        auto* next=get<P>(n,0);
        if(!count||!next||next==head){
            if(s->publish)s->publish(batch.data(),s->context);
            used=0;batch[0]=0;
        }
        xcurl_research_telemetry_unlink(n);lifetime->release(n,2,lifetime->context);
    }
    put(p,0x20,s->tick64(s->context));
}
extern "C" void xcurl_research_telemetry_destroy_local(void* p,const TelemetryServices* s){
    if(p){xcurl_research_telemetry_destroy_records(p,s);s->release(p,3,s->context);}
}
extern "C" void xcurl_research_telemetry_format_guid(const std::uint8_t* g,char16_t* out){
    constexpr unsigned order[]={3,2,1,0,45,5,4,45,7,6,45,8,9,45,10,11,12,13,14,15};
    constexpr char16_t hex[]=u"0123456789ABCDEF";
    for(auto i:order){if(i==45)*out++=u'-';else{*out++=hex[g[i]>>4];*out++=hex[g[i]&15];}}
    *out=0;
}
