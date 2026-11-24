// Original Store 0x102FC and 0xC070, ETW disabled.
#include "upload_buffer.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
}
extern "C" std::uint32_t xcurl_research_allocate_upload(void* request,void** out,std::uint64_t size,const BufferServices* b){
    auto* s=b->request;auto c=s->context;*out=s->allocate(static_cast<std::size_t>(size),16,c);
    if(*out)return 0;
    auto* e=get<P>(request,0x30);b->diagnose(e,0x21778,size,0,c);xcurl_research_easy_cancel(e,0x8007000eU,27,s);xcurl_research_request_stop(request,s);return 0x8007000eU;
}
extern "C" std::uint32_t xcurl_research_buffer_upload(void* easy,void** out,std::uint64_t* cap,std::uint64_t* length,const BufferServices* b){
    auto* e=static_cast<P>(easy);auto* s=b->request;auto c=s->context;
    s->enter(e+0x50,c);auto size=get<std::uint64_t>(e,0xf0);auto post=get<std::uint64_t>(e,0xf8);auto block=get<std::uint32_t>(e,0xb0);auto cb=get<TransferCallback>(e,0x120);auto user=get<void*>(e,0x118);s->leave(e+0x50,c);
    *out=nullptr;*cap=0;*length=0;if((!size&&!post)||!cb)return 0;
    auto status=xcurl_research_allocate_upload(e+0x230,out,block,b);if(status&0x80000000U)return status;
    std::uint64_t total=0, allocated=block;
    auto fail=[&](std::uint32_t error,std::uint32_t code){xcurl_research_easy_cancel(e,error,code,s);xcurl_research_request_stop(e+0x230,s);s->release(*out,16,c);*out=nullptr;*cap=0;*length=0;return error;};
    for(;;){
        // Original resets the recorded capacity to the configured block every
        // iteration and performs unsigned subtraction, even after growth.
        *cap=block;
        if(total>UINT64_MAX-block)return fail(0x8007000eU,27);
        // Do not reproduce the original unsigned-underflow allocation overrun.
        // In the normal domain this is exactly the original allocation path.
        if(*cap-total<block||total+block>allocated){auto target=total+block;auto* p=b->reallocate(*out,static_cast<std::size_t>(target),16,c);if(!p){b->diagnose(e,0x209c0,*cap,target,c);return fail(0x8007000eU,27);}*out=p;*cap=target;allocated=target;}
        auto n=cb(static_cast<P>(*out)+total,1,block,user);
        if(n==0x10000000ULL||n==0x10000001ULL){b->diagnose(e,0x20a08,0,0,c);return fail(0x80004004U,42);}
        if(!n){*length=total;return 0;}
        // Original trusts the callback's size and can overrun its allocation.
        // Fail closed for invalid callback sizes; equivalence tests restrict this domain.
        if(n>block||total>UINT64_MAX-n){b->diagnose(e,0x20a08,0,0,c);return fail(0x80004004U,42);}
        total+=n;
    }
}
