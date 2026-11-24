// Original Store XCurl 0x12868 and 0xC9C0, disabled ETW.
#include "upload_engine.h"
#include <bit>
#include <charconv>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<P>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(void* p,unsigned o){return get<P>(p,o);}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
bool failed(std::uint32_t n){return (n&0x80000000U)!=0;}
}
extern "C" void xcurl_research_upload_continue(void* request,const UploadServices* u){
    auto* p=static_cast<P>(request);auto* s=u->request;auto c=s->context;
    s->enter(p,c);bool active=get<std::uint32_t>(p,0x28)==2;s->leave(p,c);if(!active)return;
    auto* e=ptr(p,0x30);
    auto fail=[&](unsigned site,std::uint64_t a,std::uint64_t b,std::uint32_t h,std::uint32_t code){u->diagnose(e,site,a,b,c);xcurl_research_easy_cancel(e,h,code,s);xcurl_research_request_close(p,s);};
    auto* buffer=ptr(p,0xb0);
    if(!buffer){auto size=static_cast<std::uint64_t>(get<std::uint32_t>(e,0xb0))+12;put(p,0xa8,size);buffer=static_cast<P>(s->allocate(size,16,c));put(p,0xb0,buffer);}
    if(!buffer){fail(0x213e8,0x8007000eU,0,0x8007000eU,55);return;}
    auto capacity=get<std::uint64_t>(p,0xa8)-12;
    auto* payload=buffer+10;auto n=xcurl_research_invoke_read(e,payload,capacity,s);
    if(n==0x10000000ULL||n==0x10000001ULL){fail(0x20a08,0,0,0x80004004U,42);return;}
    if(n>capacity){fail(0x21378,n,capacity,0x8007006fU,42);return;}
    auto mode=get<std::uint32_t>(p,0xd0);auto* data=payload;bool done=false;std::uint32_t status=0;
    if(!n){data=buffer;done=true;
        if(ptr(e,0x140)){status=u->trailers(p,&n,c);if(failed(status))xcurl_research_request_close(p,s);}
        else if(mode==2){std::memcpy(buffer,"0\r\n\r\n",5);n=5;}
    }else if(mode==2){
        // Original formats "%zx%s" followed by CRLF, with up to 8 hex digits
        // (capacity derives from a 32-bit configured buffer size).
        char prefix[11];auto [end,ec]=std::to_chars(prefix,prefix+8,n,16);
        if(ec!=std::errc{}){fail(0x21378,n,capacity,0x8007006fU,42);return;}
        *end++='\r';*end++='\n';auto prefix_len=static_cast<std::size_t>(end-prefix);
        data=payload-prefix_len;std::memcpy(data,prefix,prefix_len);std::memcpy(payload+n,"\r\n",2);n+=prefix_len+2;
    }else if(mode==1){auto sent=get<std::uint64_t>(p,0xa0)+n;put(p,0xa0,sent);auto total=get<std::int64_t>(e,0xf8);if(get<std::uint32_t>(e,0x80)!=2||total<=0)total=get<std::int64_t>(e,0xf0);done=sent==static_cast<std::uint64_t>(total);}
    if(done){p[0xd5]=1;if(failed(status))return;}
    if(e[0x176]&&data)s->debug_data(e,4,data,n,c);
    if(!u->write(ptr(p,0x50),n?data:nullptr,static_cast<std::uint32_t>(n),c)){auto h=hr(s->last_error(c));fail(0x213b0,h,0,h,55);}
}
extern "C" void xcurl_research_upload_worker(void* easy,const UploadServices* u){xcurl_research_upload_continue(static_cast<P>(easy)+0x230,u);}
