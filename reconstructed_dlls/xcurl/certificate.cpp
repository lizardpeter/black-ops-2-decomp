// Store original 0x10CA0 and 0x11478, disabled ETW.
#include "certificate.h"
#include <bit>
#include <cstring>
#include <string>
#include <string_view>
using namespace t6_xcurl;
namespace {
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
constexpr std::u16string_view locations[]={u"CurrentUser",u"LocalMachine",u"CurrentService",u"Services",u"Users",u"CurrentUserGroupPolicy",u"LocalMachineGroupPolicy",u"LocalMachineEnterprise"};
constexpr std::uint32_t flags[]={0x10000,0x20000,0x40000,0x50000,0x60000,0x70000,0x80000,0x90000};
}
extern "C" std::uint32_t xcurl_research_parse_certificate(const char16_t* input,CertificateSelector* out){
    std::u16string_view s(input);auto first=s.find(u'\\');if(first==s.npos)return 58;
    bool found=false;for(unsigned i=0;i<8;++i)if(locations[i].starts_with(s.substr(0,first))){out->location=flags[i];found=true;break;}
    if(!found)return 58;
    auto second=s.find(u'\\',first+1);if(second==s.npos)return 58;
    // Borrow immutable spans instead of the original temporary input mutation and MSVC string allocation.
    out->store=input+first+1;out->store_length=second-first-1;out->thumbprint=input+second+1;
    return s.size()-second-1==40?0:58;
}
extern "C" std::uint32_t xcurl_research_select_certificate(void* request,const char16_t* selector,const CertificateServices* s){
    if(!selector)return 0;
    auto* easy=get<void*>(request,0x30);auto c=s->context;std::uint32_t status=0;CertificateSelector parsed{};
    if(xcurl_research_parse_certificate(selector,&parsed)){status=0x80092004U;s->diagnose(easy,0x21130,0,selector,0,c);}
    else{
        std::u16string store(parsed.store,parsed.store_length);auto* h=s->open(parsed.location|0x4000,store.c_str(),c);
        if(!h){auto err=s->error(c);status=std::bit_cast<std::int32_t>(err)>0?(err&0xffffU)|0x80070000U:err;s->diagnose(easy,0x210f8,parsed.location,store.c_str(),status,c);}
        else{
            std::uint8_t hash[20]{};std::uint32_t n=20;void* cert=nullptr;
            if(s->decode(parsed.thumbprint,40,hash,&n,c))cert=s->find(h,hash,n,c);
            if(cert){s->set_certificate(get<void*>(request,0x50),cert,40,c);s->free_certificate(cert,c);}
            else status=0x80092004U;
            s->close_store(h,c);
        }
    }
    if(status&0x80000000U)s->diagnose(easy,0x21168,status,nullptr,0,c);
    return status;
}
