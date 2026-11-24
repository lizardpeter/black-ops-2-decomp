// Original XCurl Store x64 0x10748, 0x12FD8, 0x131A8. ETW disabled.
#include "http_setup.h"
#include <bit>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
bool failed(std::uint32_t x){return (x&0x80000000U)!=0;}
}
extern "C" void xcurl_research_upload_mode(void* request,std::uint32_t mode){put(request,0xd0,mode);}
extern "C" std::uint32_t xcurl_research_http_open(void* request,void* easy,const char16_t* method,void* url,const char16_t* const* accepts,std::uint16_t port,const HttpSetupServices* s){
    auto c=s->context;
    auto* host=get<char16_t*>(url,0x18);auto index=get<std::uint32_t>(url,0x20);auto saved=host[index];host[index]=0;
    auto* conn=s->connect(get<void*>(get<void*>(request,0x40),0x10),host,port,c);
    // Reload both fields because the original reloads them after the call.
    get<char16_t*>(url,0x18)[get<std::uint32_t>(url,0x20)]=saved;
    if(!conn){auto x=hr(s->error(c));s->diagnostic(easy,0x21610,x,c);return x;}
    put(request,0x48,conn);std::uint32_t flags=get<std::uint32_t>(url,0x14)==1?0:0x800000;
    auto* owner=get<void*>(request,0x30);
    if(get<std::int32_t>(owner,0x180)>=3&&get<std::int64_t>(owner,0xf0)==-1){flags|=0x200;xcurl_research_upload_mode(request,3);}
    auto open=[&]{return s->open(conn,method,get<const char16_t*>(url,0x48),accepts,flags,c);};
    auto* handle=open();
    if(!handle){auto err=s->error(c);if(err==87&&(flags&0x200)){flags&=~0x200U;xcurl_research_upload_mode(request,0);s->retry_diagnostic(easy,c);handle=open();}}
    if(handle){put(request,0x50,handle);return 0;}
    auto x=hr(s->error(c));s->diagnostic(easy,0x215e0,x,c);return x;
}
extern "C" std::uint32_t xcurl_research_http_options(void* request,const void* opt,std::uint32_t scheme,const HttpSetupServices* s){
    auto c=s->context;std::uint32_t status=0;
    auto set=[&](unsigned n,unsigned v,unsigned site){
        if(!s->option(get<void*>(request,0x50),n,v,c)){status=hr(s->error(c));s->diagnostic(get<void*>(request,0x30),site,status,c);return false;}return true;
    };
    auto version=get<std::uint32_t>(opt,0x108);
    auto protocol=version==0||version==3||(version==4&&scheme!=1)?1U:0U;
    if(!set(0x85,protocol,0x21348))return status;
    if(!get<std::uint8_t>(opt,0xfb)){
        if(!set(63,2,0x211c8)&&failed(status))return status;
    }else{
        if(!set(88,1,0x21200)&&failed(status))return status;
        if(!set(89,get<std::uint32_t>(opt,0x30),0x21240)&&failed(status))return status;
    }
    if(auto decompression=get<std::uint32_t>(opt,0x68))if(!set(118,decompression,0x21278)&&failed(status))return status;
    auto timeout=get<std::uint32_t>(opt,0x28);
    if(!s->timeouts(get<void*>(request,0x50),0,get<std::uint32_t>(opt,0x2c),timeout,timeout,c)){
        status=hr(s->error(c));s->diagnostic(get<void*>(request,0x30),0x212a8,status,c);if(failed(status))return status;
    }
    if(get<std::uint8_t>(request,0xd6))if(!set(31,0x100,0x212e0)&&failed(status))return status;
    if(!set(63,1,0x21308)&&failed(status))return status;
    if(get<void*>(opt,0xa8)&&!get<std::uint32_t>(request,0xd0)){
        auto method=get<std::uint32_t>(opt,8);
        if((get<std::int64_t>(opt,0x78)>0&&(method==1||method==2))||(get<std::int64_t>(opt,0x78)<=0&&get<std::int64_t>(opt,0x80)>0&&method==2))xcurl_research_upload_mode(request,1);
    }
    return status;
}
// 0x137B8: acquire session, select effective port/method, then open.
// acquire is the original 0x15F40 boundary; its engine+0x50 argument is retained.
extern "C" std::uint32_t xcurl_research_http_open_with_session(void* request,void* easy,void* url,void* engine,AcquireSession acquire,void* acquire_context,const HttpSetupServices* s){
    auto* output=static_cast<P>(request)+0x40;
    void* session=get<void*>(request,0x40);
    auto status=acquire(static_cast<P>(engine)+0x50,easy,get<std::uint32_t>(get<void*>(request,0x98),0),get<std::uint32_t>(url,0x14),&session,acquire_context);
    std::memcpy(output,&session,sizeof(session));
    if(failed(status))return status;
    auto port=get<std::uint16_t>(easy,0x90);if(!port)port=get<std::uint16_t>(url,0x24);
    auto* method=get<const char16_t*>(easy,0xc8);
    static constexpr const char16_t* methods[]={u"GET",u"PUT",u"POST",u"HEAD"};
    if(!method)method=methods[get<std::uint32_t>(easy,0x80)]; // original requires a valid method index
    return xcurl_research_http_open(request,easy,method,url,get<const char16_t* const*>(easy,0xe8),port,s);
}
// 0x94AC: independent copy, does not free the previous output pointer.
extern "C" std::uint32_t xcurl_research_copy_utf16(const char16_t* src,void** out,OptionAllocate allocate,void* context){
    auto* end=src;while(*end)++end;
    auto n=static_cast<std::size_t>(end-src+1)*sizeof(char16_t);
    *out=allocate(n,7,context);if(!*out)return 0x8007000eU;
    std::memmove(*out,src,n);return 0;
}
