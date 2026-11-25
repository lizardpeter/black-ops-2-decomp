// Original Store XCurl.dll 0xC6A0, 0xD174, 0x11780, 0xCF90; ETW disabled.
#include "request_prepare.h"
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(P p,unsigned o){T v;std::memcpy(&v,p+o,sizeof(v));return v;}
template<class T>void put(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
bool failed(std::uint32_t v){return (v&0x80000000U)!=0;}
P ptr(P p,unsigned o){return get<P>(p,o);}
}
extern "C" void xcurl_research_easy_reset_transfer(void* easy,const RequestServices* s){
    auto* e=static_cast<P>(easy);auto c=s->context;
    s->enter(e+0x10,c);s->enter(e+0x188,c);
    if(auto* p=ptr(e,0x1b8))s->release(p,7,c);
    if(auto* p=ptr(e,0x1d0))s->release(p,8,c);
    if(auto* p=ptr(e,0x218))s->release(p,8,c);
    if(auto* p=ptr(e,0x208))s->release(p,8,c);
    // Original copies a temporary aggregate. Its unwritten padding is not a
    // semantic field; deterministically zero that padding instead of exposing stack data.
    std::memset(e+0x1b0,0,0x80);put(e,0x1f8,UINT64_MAX);
    put<P>(e,0x338,nullptr);std::memset(e+0x38,0,0x18);
    s->leave(e+0x188,c);s->leave(e+0x10,c);
}
extern "C" std::uint32_t xcurl_research_easy_prepare_url(void* easy,const PrepareServices* s){
    auto* e=static_cast<P>(easy);auto* r=s->request;auto c=r->context;
    r->enter(e+0x188,c);r->enter(e+0x50,c);
    std::uint32_t status=0;
    auto* url=ptr(e,0x88);
    if(!ptr(e,0x1b8)){
        if(url){status=s->prepare_url(url,e,e+0x1b8,c);if(failed(status))s->url_diagnostic(e,0x207e8,reinterpret_cast<std::uintptr_t>(url),status,c);}
        if(!failed(status)&&ptr(e,0x78)){
            if(!url){void* text=nullptr;auto code=s->default_url(ptr(e,0x78),0,&text,c);
                if(code){status=0x80072ee5U;s->url_diagnostic(e,0x20810,code,0,c);}
                else{status=s->prepare_url(text,e,e+0x1b8,c);if(failed(status))s->url_diagnostic(e,0x207e8,reinterpret_cast<std::uintptr_t>(text),status,c);if(text)r->release(text,5,c);}
            }else if(!get<std::uint16_t>(e,0x90)){void* text=nullptr;auto code=s->default_url(ptr(e,0x78),6,&text,c);
                if(code){status=0x80072ee5U;s->url_diagnostic(e,0x20848,code,0,c);}
                else{auto port=s->parse_port(text,c);if(text)r->release(text,5,c);s->url_diagnostic(e,0x20830,port,0,c);put(e,0x90,static_cast<std::uint16_t>(port));}
            }
        }
    }
    r->leave(e+0x50,c);r->leave(e+0x188,c);
    if(failed(status)){xcurl_research_easy_cancel(e,status,status==0x8007000eU?27:3,r);xcurl_research_request_stop(e+0x230,r);}
    return status;
}
extern "C" void xcurl_research_request_begin(void* request,const PrepareServices* s){
    auto* p=static_cast<P>(request);auto* r=s->request;auto c=r->context;
    r->enter(p,c);auto state=get<std::uint32_t>(p,0x28);bool finish=state==3;
    if(!state){std::memset(p+0x58,0,0x38);put(p,0x58,s->engine_queue);put(p,0x60,p);put(p,0x68,s->start_callback);put(p,0x28,1U);
        auto status=xcurl_research_gdk_network_security_async(reinterpret_cast<const char16_t*>(ptr(p,0x38)),p+0x58,s->gdk);
        if(failed(status)){r->diagnostic(ptr(p,0x30),0x20f80,status,c);xcurl_research_easy_cancel(ptr(p,0x30),status,58,r);finish=true;}
    }
    r->leave(p,c);if(finish)xcurl_research_request_finish(p,r);
}
extern "C" std::uint32_t xcurl_research_easy_begin(void* easy,const PrepareServices* s){
    auto* e=static_cast<P>(easy);auto* r=s->request;auto c=r->context;
    xcurl_research_easy_reset_transfer(e,r);auto status=xcurl_research_easy_prepare_url(e,s);
    if(failed(status))r->diagnostic(e,0x207b8,status,c);
    auto* p=e+0x230;
    auto* url=ptr(e,0x1b8);
    if(url&&failed(s->copy_url(url,p+0x38,c))){r->diagnostic(ptr(p,0x30),0x20de8,0,c);xcurl_research_easy_cancel(ptr(p,0x30),0x8007000eU,27,r);xcurl_research_request_finish(p,r);}
    else xcurl_research_request_begin(p,s);
    return get<std::uint32_t>(e,0x220);
}
