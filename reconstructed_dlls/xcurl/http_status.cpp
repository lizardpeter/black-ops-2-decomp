// XCurl Store 13E14: WinHTTP notification dispatcher, ETW disabled.
#include "http_status.h"
#include "engine_lifecycle.h"
#include <bit>
#include <cstring>
using namespace t6_xcurl;
namespace {
using P=std::uint8_t*;
template<class T>T get(const void* p,unsigned o=0){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
template<class T>void put(P p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
bool failed(std::uint32_t x){return (x&0x80000000U)!=0;}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
}
namespace {
void http_status(void* request,std::uint32_t status,const void* info,std::uint32_t length,const StatusServices* s,bool bound){
    auto* p=static_cast<P>(request);auto* r=s->request;auto ctx=r->context;
    auto easy=[&]{return get<P>(p,0x30);};
    // WinHTTP can notify an aborted pending read after a user callback has
    // already canceled and closed the request. Do not let that notification
    // overwrite the first failure text, enqueue more work or close twice.
    if(bound&&status!=0x800){
        r->enter(p,ctx);auto state=get<std::uint32_t>(p,0x28);r->leave(p,ctx);
        if(state>=5||state==0)return;
        if(status==0x200000&&(!info||length<16))return;
        if(status==0x40000&&(!info||length<4))return;
        if(status==0x4000&&(!info||length<2||length%2))return;
    }
    auto close=[&]{xcurl_research_request_close(p,r);};
    auto diag=[&](unsigned site,std::uint64_t a,std::uint64_t b=0,std::uint32_t c=0){s->diagnostic(easy(),site,a,b,c,ctx);};
    auto cancel=[&](std::uint32_t h,unsigned code){xcurl_research_easy_cancel(easy(),h,code,r);close();};
    auto fail=[&](unsigned site,std::uint32_t h,unsigned code){
        if(bound){r->enter(easy()+0x188,ctx);bool earlier=failed(get<std::uint32_t>(easy(),0x228));r->leave(easy()+0x188,ctx);if(earlier){close();return;}}
        diag(site,h);cancel(h,code);
    };
    auto release=[&](unsigned off,unsigned tag){if(auto* v=get<P>(p,off)){r->release(v,tag,ctx);put<P>(p,off,nullptr);}};
    auto query=[&]{if(!s->query_available(get<P>(p,0x50),ctx))fail(0x21868,hr(r->last_error(ctx)),56);};
    auto eof=[&](unsigned site){auto received=get<std::uint64_t>(p,0xc8),expected=get<std::uint64_t>(p,0xc0);if(p[0xb8]&&get<std::uint32_t>(easy(),0x80)!=3&&!easy()[0x174]&&received<expected){diag(site,received,expected,0x8007000dU);cancel(0x8007000dU,18);}else close();};
    switch(status){
    case 0x10:{auto h=xcurl_research_verify_server_certificate(p,s->gdk);if(failed(h))fail(0x217a8,h,60);break;}
    case 0x800:put<P>(p,0x50,nullptr);xcurl_research_request_finish(p,r);break;
    case 0x4000:{release(0x38,7);auto h=s->redirect(info,p+0x38,ctx);if(failed(h)){diag(0x219a8,reinterpret_cast<std::uintptr_t>(info));cancel(h,27);}break;}
    case 0x200000:{auto api=get<std::uint64_t>(info);auto error=get<std::uint32_t>(info,8);unsigned code=55,site=0x21ae0;
        if(error==12002){code=28;site=0x219d8;}
        else if(api>=1&&api<=3){if(error==12156){code=47;site=0x21a00;}else if(api==1){code=52;site=0x21a20;}else{code=56;site=0x21a68;}}
        else if(api==4){code=23;site=0x21a90;}else if(api==5)site=0x21ab8;
        fail(site,hr(error),code);break;}
    case 0x100000:case 0x400000:{if(get<std::uint32_t>(p,0xd0)&&!p[0xd5]){auto h=s->continue_upload(easy(),ctx);if(failed(h))fail(0x21810,h,55);}else{release(0xb0,16);auto h=xcurl_research_http_receive(p,r);if(failed(h))fail(0x217e0,h,55);}break;}
    case 0x20000:{std::uint32_t http=0;auto h=s->headers(p,easy(),&http,ctx);if(failed(h)){cancel(h,22);break;}h=s->publish_headers(easy(),ctx);if(failed(h)){fail(0x21898,h,22);break;}if(easy()[0x177]&&http>=400){diag(0x21840,http);cancel(http|0x80190000U,22);}else query();break;}
    case 0x80000:{put(p,0xc8,get<std::uint64_t>(p,0xc8)+length);if(length){auto h=s->publish_data(easy(),info,length,ctx);if(failed(h))fail(0x21940,h,56);else query();}else eof(0x21970);break;}
    case 0x40000:{auto n=get<std::uint32_t>(info);put(p,0xdc,n);if(!n){eof(0x21900);break;}if(p[0xb8]&&!easy()[0x174]&&get<std::uint64_t>(p,0xc8)+n>get<std::uint64_t>(p,0xc0)){fail(0x218c8,0x8007000dU,56);break;}auto h=s->schedule_read(p,easy(),ctx);if(failed(h))fail(0x208e8,h,56);break;}
    default:break;
    }
}

}
extern "C" void xcurl_research_http_status(void* p,std::uint32_t n,const void* i,std::uint32_t l,const StatusServices* s){http_status(p,n,i,l,s,false);}
extern "C" void xcurl_bound_http_status(void* p,std::uint32_t n,const void* i,std::uint32_t l,const StatusServices* s){http_status(p,n,i,l,s,true);}
