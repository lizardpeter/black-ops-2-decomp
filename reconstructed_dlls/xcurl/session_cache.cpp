// Original Store XCurl 0x1A9D0/0x1A860, 0x1A9C0, 0x1AB90,
// 0x15C58, 0x15E9C/0x159F0, 0x15F40, 0x15B64; ETW disabled.
#include "session_cache.h"
#include <atomic>
#include <bit>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(void* p,unsigned o){return get<P>(p,o);}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
bool failed(std::uint32_t x){return (x&0x80000000U)!=0;}
bool equal(const char16_t* a,const char16_t* b){while(*a==*b){if(!*a)return true;++a;++b;}return false;}
void self(P n){put(n,0,n);put(n,8,n);}
void push_back(P h,P n){put(n,0,h);put(n,8,ptr(h,8));put(ptr(h,8),0,n);put(h,8,n);}
void unlink(P n){put(ptr(n,0),8,ptr(n,8));put(ptr(n,8),0,ptr(n,0));self(n);}
std::atomic_ref<std::uint32_t> count(void* p){return std::atomic_ref(*reinterpret_cast<std::uint32_t*>(static_cast<P>(p)+8));}
}
extern "C" std::uint32_t xcurl_research_session_create(void* handle,void** out,const SessionServices* s){
    auto* p=s->allocate(0x20,17,s->context);if(!p)return 0x8007000e;
    put(p,8,1U);put(p,0,s->vtable_identity);put(p,0x10,handle);put(p,0x18,std::uint8_t(0));*out=p;return 0;
}
extern "C" std::uint32_t xcurl_research_session_addref(void* p){return count(p).fetch_add(1)+1;}
extern "C" std::uint32_t xcurl_research_session_release(void* p,const SessionServices* s){
    auto n=count(p).fetch_sub(1)-1;if(!n){put(p,0,s->vtable_identity);if(auto* h=ptr(p,0x10)){s->close(h,s->context);put<P>(p,0x10,nullptr);}s->release(p,17,s->context);}return n;
}
extern "C" void xcurl_research_cache_entry_destroy(void* p,const SessionServices* s){
    if(auto* session=ptr(p,0x28))xcurl_research_session_release(session,s);
    if(auto* agent=ptr(p,0x18)){s->release(agent,7,s->context);put<P>(p,0x18,nullptr);}s->release(p,19,s->context);
}
extern "C" std::uint32_t xcurl_research_cache_entry_create(std::uint32_t tls,const char16_t* agent,std::uint32_t scheme,void* handle,void** out,const SessionServices* s){
    auto* p=static_cast<P>(s->allocate(0x30,19,s->context));if(!p)return 0x8007000e;
    std::memset(p+0x10,0,0x20);self(p);put(p,0x10,tls);put(p,0x20,scheme);
    void* copy=nullptr;auto status=xcurl_research_copy_utf16(agent,&copy,s->allocate,s->context);put(p,0x18,copy);
    if(!failed(status)){void* session=nullptr;status=xcurl_research_session_create(handle,&session,s);put(p,0x28,session);}
    if(failed(status))xcurl_research_cache_entry_destroy(p,s);else *out=p;return status;
}
namespace {
std::uint32_t acquire_session(void* cache,void* easy,std::uint32_t tls,std::uint32_t scheme,void** out,const SessionServices* s,bool safe){
    auto* head=static_cast<P>(cache)+0x28;auto c=s->context;s->enter(cache,c);
    auto* agent=get<const char16_t*>(easy,0xd8);if(!agent)agent=u"XCURL/2403.2.0.0.amd64fre";
    P found=nullptr;std::uint32_t status=0;
    for(auto* n=ptr(head,0);n&&n!=head;n=ptr(n,0))if(get<std::uint32_t>(n,0x10)==tls&&equal(get<const char16_t*>(n,0x18),agent)&&get<std::uint32_t>(n,0x20)==scheme)found=n;
    if(!found){
        const bool secure=scheme!=1;
        auto* handle=s->open_session(agent,secure?0x30000000U:0x10000000U,c);
        if((!safe||!handle)&&s->error(c)==87)handle=s->open_session(agent,0x10000000U,c);
        if(handle){
            if(secure&&!s->set_option(handle,84,tls,c)){status=hr(s->error(c));if(safe&&!failed(status))status=0x80004005U;s->diagnostic(easy,0x21b90,status,c);if(safe&&failed(status))s->close(handle,c); }
            // As in the original, option failure does not close the handle here.
            if(!failed(status)){
                void* entry=nullptr;status=xcurl_research_cache_entry_create(tls,agent,scheme,handle,&entry,s);found=static_cast<P>(entry);
                if(failed(status)){s->close(handle,c);s->diagnostic(easy,0x21bc0,status,c);}else push_back(head,found);
            }
        }else{status=hr(s->error(c));if(safe&&!failed(status))status=0x80004005U;s->diagnostic(easy,0x21bf0,status,c);}
    }
    // The bound path fails closed if a host has not supplied a valid entry.
    if(safe&&!failed(status)&&!found)status=0x80004005U;
    if(!failed(status)){*out=ptr(found,0x28);xcurl_research_session_addref(*out);}
    s->leave(cache,c);return status;
}
}
extern "C" std::uint32_t xcurl_research_session_acquire(void* cache,void* easy,std::uint32_t tls,std::uint32_t scheme,void** out,const SessionServices* s){return acquire_session(cache,easy,tls,scheme,out,s,false);}
extern "C" std::uint32_t xcurl_bound_session_acquire(void* cache,void* easy,std::uint32_t tls,std::uint32_t scheme,void** out,const SessionServices* s){return acquire_session(cache,easy,tls,scheme,out,s,true);}
extern "C" void xcurl_research_session_cache_clear(void* cache,const SessionServices* s){
    auto c=s->context;s->enter(cache,c);auto* head=static_cast<P>(cache)+0x28;
    while(auto* n=ptr(head,0)){if(n==head)break;unlink(n);xcurl_research_cache_entry_destroy(n,s);}
    s->leave(cache,c);
}
