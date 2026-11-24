// Original Store XCurl 0x14D04, 0x1ABB0, 0x14F84, 0xCD4C. ETW disabled.
#include "header_query.h"
#include <bit>
#include <cstring>
#include <limits>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
bool failed(std::uint32_t n){return (n&0x80000000U)!=0;}
}
extern "C" std::uint32_t xcurl_research_query_wide(void* handle,std::uint32_t level,void** out,std::uint64_t* bytes,const HeaderQueryServices* s){
    auto c=s->context;std::uint32_t n=0;
    (void)s->query(handle,level,nullptr,&n,c);
    auto status=hr(s->error(c));
    if(status!=0x8007007aU)return failed(status)?status:0x8000ffffU;
    auto* buffer=s->allocate(n,8,c);if(!buffer)return 0x8007000eU;
    if(s->query(handle,level,buffer,&n,c)){*out=buffer;*bytes=n;return 0;}
    status=hr(s->error(c));if(!failed(status))status=0x8000ffffU;
    s->release(buffer,8,c);return status;
}
extern "C" std::uint32_t xcurl_research_header_utf8(const char16_t* source,void** out,std::uint64_t* bytes,const HeaderQueryServices* s){
    const auto* end=source;while(*end)++end;
    const auto units=end-source;
    if(units>std::numeric_limits<int>::max())return 27;
    auto c=s->context;auto n=s->utf8(source,static_cast<int>(units),nullptr,0,c);
    // Original allocates one byte after failed conversion and then writes at
    // the UTF-16 length, potentially out of bounds. Reject that invalid domain
    // explicitly; differential equivalence is for valid UTF-16 inputs.
    if(n<units||n<0)return 27;
    auto* buffer=static_cast<char*>(s->allocate(static_cast<std::size_t>(n)+1,8,c));if(!buffer)return 27;
    (void)s->utf8(source,static_cast<int>(units),buffer,n,c);
    // Verified original quirk: terminates at UTF-16 code-unit length rather than UTF-8 byte length.
    buffer[units]=0;*out=buffer;*bytes=static_cast<std::uint64_t>(n);return 0;
}
extern "C" std::uint32_t xcurl_research_query_utf8(void* handle,std::uint32_t level,void** out,std::uint64_t* bytes,const HeaderQueryServices* s){
    void* wide=nullptr;std::uint64_t n=0;auto status=xcurl_research_query_wide(handle,level,&wide,&n,s);
    if(failed(status))return status;
    if(xcurl_research_header_utf8(static_cast<const char16_t*>(wide),out,bytes,s))status=0x8007000eU;
    if(wide)s->release(wide,8,s->context);return status;
}
extern "C" void xcurl_research_publish_header_fields(void* easy,void** effective,std::uint32_t code,std::uint8_t known,std::uint64_t content_length,std::uint64_t header_length,void** headers,std::uint32_t version,const char* scheme,std::uint32_t protocol,void** type,std::uint32_t extra,const RequestServices* s){
    auto* e=static_cast<P>(easy);auto c=s->context;s->enter(e+0x188,c);
    put(e,0x208,*effective);*effective=nullptr;put(e,0x1d0,*headers);*headers=nullptr;put(e,0x218,*type);*type=nullptr;
    put(e,0x1e0,code);put(e,0x1f4,known);put(e,0x1f8,content_length);put(e,0x1dc,version);
    put(e,0x210,scheme);put(e,0x1e8,static_cast<std::uint64_t>(extra));put(e,0x1c8,header_length);put(e,0x1f0,static_cast<std::uint32_t>(header_length));
    (void)protocol; // passed to ETW only in original
    s->leave(e+0x188,c);
}
