// Original Store XCurl 0x11978 and 0x11F10, disabled ETW.
#include "request_start.h"
#include <algorithm>
#include <array>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<P>(p)+o,&v,sizeof(v));}
P ptr(const void* p,unsigned o){return get<P>(p,o);}
bool failed(std::uint32_t n){return (n&0x80000000U)!=0;}
}
extern "C" void xcurl_research_start_request(void* request,void* task,const StartServices* t){
    auto* p=static_cast<P>(request);auto* s=t->request;auto c=s->context;
    s->enter(p,c);unsigned action=0;
    auto easy=[&]{return ptr(p,0x30);};
    auto cancel=[&](std::uint32_t h,unsigned code){xcurl_research_easy_cancel(easy(),h,code,s);action=2;};
    auto cancel_close=[&](std::uint32_t h,unsigned code){xcurl_research_easy_cancel(easy(),h,code,s);action=1;};
    auto release_metadata=[&]{if(auto* old=ptr(p,0x90)){s->release(old,15,c);put<P>(p,0x90,nullptr);}};
    auto run=[&]{
        if(get<std::uint32_t>(p,0x28)!=1){action=2;return;}
        auto h=t->async_status(task,c);if(failed(h)){t->diagnose(easy(),0x210b8,h,c);cancel(h,58);return;}
        std::uint64_t bytes=0;h=t->async_size(task,&bytes,c);if(failed(h)){t->diagnose(easy(),0x21060,h,c);cancel(h,58);return;}
        release_metadata();auto* meta=s->allocate(bytes,15,c);put(p,0x90,meta);
        if(!meta){t->diagnose(easy(),0x21038,0,c);cancel(0x8007000eU,58);return;}
        void* parsed=ptr(p,0x98);h=t->async_result(task,bytes,meta,&parsed,c);put(p,0x98,parsed);
        if(failed(h)){t->diagnose(easy(),0x20fe0,h,c);release_metadata();cancel(h,58);return;}
        auto count=get<std::uint64_t>(parsed,8);auto* items=ptr(parsed,0x10);
        for(std::uint64_t i=0;i<count;++i)if(get<std::uint32_t>(items,0)==2){p[0xd6]=1;break;}else items+=0x18;
        alignas(8) std::array<std::uint8_t,0x68> url{};
        h=t->crack(get<const char16_t*>(p,0x38),easy(),url.data(),c);if(failed(h)){action=2;return;}
        h=t->open(p,easy(),url.data(),c);if(failed(h)){cancel(h,55);return;}
        h=t->register_request(p,c);if(failed(h)){s->close_http(ptr(p,0x50),c);put<P>(p,0x50,nullptr);cancel(h,55);return;}
        put(p,0x28,2U);
        h=t->options(p,easy()+0x78,get<std::uint32_t>(url.data(),0x14),c);if(failed(h)){cancel_close(h,55);return;}
        h=t->headers(p,ptr(easy(),0x98),c);if(failed(h)){cancel_close(h,3);return;}
        h=t->certificate(p,ptr(easy(),0xd0),c);if(failed(h)){cancel_close(h,58);return;}
        auto* e=easy();auto method=get<std::uint32_t>(e,0x80);auto mode=get<std::uint32_t>(p,0xd0);
        const void* data=nullptr;std::uint64_t n=0,total=0;
        if(method==2&&ptr(e,0xc0)){
            data=ptr(e,0xc0);n=get<std::uint64_t>(e,0xf8);if(n==UINT64_MAX)n=std::strlen(static_cast<const char*>(data));total=n;
        }else if(mode==2||mode==3||method==0||method==3){/* send without immediate body */}
        else if(mode==1){
            auto fixed=get<std::int64_t>(e,0xf8);if(method!=2||fixed<=0)fixed=get<std::int64_t>(e,0xf0);total=static_cast<std::uint64_t>(fixed);
            auto cap=std::min(total,static_cast<std::uint64_t>(get<std::uint32_t>(e,0xb0)));void* body=ptr(p,0xb0);
            h=xcurl_research_allocate_upload(p,&body,cap,t->buffer);put(p,0xb0,body);
            if(failed(h)){cancel_close(h,55);return;}
            put(p,0xa8,cap);n=xcurl_research_invoke_read(e,body,cap,s);data=body;p[0xd5]=n==total;
            // Deliberate safety guard: the original startup path trusts the callback count.
            if(n>cap){cancel_close(0x80004004U,42);return;}
        }else if(mode==0){
            void* body=nullptr;std::uint64_t cap=0;
            // Original C070 is passed the owning easy handle, not the request.
            h=xcurl_research_buffer_upload(e,&body,&cap,&n,t->buffer);put(p,0xb0,body);put(p,0xa8,cap);if(failed(h)){cancel_close(h,55);return;}data=body;total=n;
        }else return;
        h=xcurl_research_http_send(p,data,n,total,s);if(failed(h))cancel_close(h,55);
    };
    run();s->leave(p,c);if(action==1)xcurl_research_request_close(p,s);else if(action==2)xcurl_research_request_finish(p,s);
}
extern "C" void xcurl_research_start_worker(void* task,const StartServices* t){xcurl_research_start_request(ptr(task,8),task,t);}
