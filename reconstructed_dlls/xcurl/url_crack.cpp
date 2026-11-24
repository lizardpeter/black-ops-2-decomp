// Original 0x14AC0, ETW-disabled path. Full x64 URL_COMPONENTS image.
#include "url_crack.h"
#include <bit>
#include <cstring>
#include <initializer_list>
using namespace t6_xcurl;
extern "C" std::uint32_t xcurl_research_crack_url(const char16_t* url,void* easy,void* components,const UrlServices* s){
    auto* p=static_cast<unsigned char*>(components);auto c=s->context;
    std::memset(p,0,0x68);
    auto put=[&](unsigned o,std::uint32_t v){std::memcpy(p+o,&v,4);};
    put(0,0x68);for(unsigned o: {0x10U,0x20U,0x50U,0x60U})put(o,UINT32_MAX);
    if(!s->crack(url,p,c)){
        auto e=s->error(c);auto h=std::bit_cast<std::int32_t>(e)>0?(e&0xffffU)|0x80070000U:e;
        s->diagnose(easy,0x20da8,url,h,c);return h;
    }
    std::uint32_t scheme;std::memcpy(&scheme,p+0x14,4);
    if(scheme==2)return 0;
    if(scheme!=1){s->diagnose(easy,0x20d70,nullptr,0,c);return 0x80072ee5U;}
    s->diagnose(easy,0x20d10,url,0,c);
    auto* report=s->format_warning(url,c);
    if(!report){s->diagnose(easy,0x20d38,nullptr,0,c);return 0x8007000eU;}
    s->report(0x80072ee5U,report,c);s->release(report,5,c);return 0;
}
