// Original Store XCurl 0x12228. ETW disabled, real query helpers and metadata publication.
#include "response_headers.h"
#include <bit>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
bool failed(std::uint32_t n){return (n&0x80000000U)!=0;}
}
extern "C" std::uint32_t xcurl_research_receive_headers(void* request,void* easy,std::uint32_t* code,const ReceiveHeaderServices* s){
    auto* p=static_cast<P>(request);auto* q=s->query;auto* r=s->request;auto c=r->context;
    void* raw=nullptr;void* type=nullptr;void* effective=nullptr;
    std::uint64_t rawlen=0,typelen=0,urlbytes=0;
    std::uint32_t status=0,version=0,extra=0,protocol=0,schemeid=0;const char* scheme=nullptr;
    auto query_number=[&](std::uint32_t level,std::uint32_t* out){std::uint32_t bytes=4;return q->query(get<void*>(p,0x50),level,out,&bytes,q->context);};
    auto run=[&]{
        if(!query_number(0x20000013,code)){
            status=hr(q->error(q->context));s->diagnose(easy,0x21750,status,0,c);return;
        }
        std::uint32_t content=0;
        if(query_number(0x20000005,&content)&&content){p[0xb8]=1;put(p,0xc0,static_cast<std::uint64_t>(content));}
        status=xcurl_research_query_utf8(get<void*>(p,0x50),0x16,&raw,&rawlen,q);
        if(failed(status)){s->diagnose(easy,0x21728,status,0,c);return;}
        auto* str=static_cast<const char*>(raw);
        if(!std::strncmp(str,"HTTP/1.0",8))version=1;
        else if(!std::strncmp(str,"HTTP/1.1",8))version=2;
        else if(!std::strncmp(str,"HTTP/2",6))version=3;
        (void)xcurl_research_query_utf8(get<void*>(p,0x50),1,&type,&typelen,q);
        (void)query_number(0x20000024,&extra);
        auto* url=get<const char16_t*>(p,0x38);
        auto converted=xcurl_research_header_utf8(url,&effective,&urlbytes,q);
        if(converted){status=0x8007000eU;s->diagnose(easy,0x216f8,converted,0,c);return;}
        status=s->crack_scheme(url,get<void*>(p,0x30),&schemeid,c);
        if(failed(status)){s->diagnose(easy,0x216d0,reinterpret_cast<std::uintptr_t>(url),status,c);return;}
        if(schemeid==1){scheme="http";protocol=1;}else if(schemeid==2){scheme="https";protocol=2;}
    };
    run();
    if(!failed(status))xcurl_research_publish_header_fields(get<void*>(p,0x30),&effective,*code,p[0xb8],get<std::uint64_t>(p,0xc0),rawlen,&raw,version,scheme,protocol,&type,extra,r);
    if(raw)q->release(raw,8,q->context);
    if(type)q->release(type,8,q->context);
    if(effective)q->release(effective,8,q->context);
    return status;
}
