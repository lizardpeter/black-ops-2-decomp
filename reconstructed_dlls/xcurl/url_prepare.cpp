// Original Store XCurl.dll 0x10EA4; ETW-disabled control flow.
// Evidence: instruction runs 45545280844 and 45546001831.
#include "url_prepare.h"
#include "wide_string.h"
#include <array>
#include <cstring>
using namespace t6_xcurl;
extern "C" std::uint32_t xcurl_research_prepare_url(const char* input,void* easy,void* out,const UrlPreparationServices* s){
    constexpr std::uint32_t invalid=0x80072ee5U,oom=0x8007000eU;
    auto c=s->context;
    char16_t* wide=nullptr;char* rebuilt=nullptr;
    std::uint32_t status=0;
    auto diag=[&](std::uint32_t site,std::uintptr_t arg=0,std::uint32_t code=0){s->diagnose(easy,site,arg,code,c);};
    auto fail=[&](std::uint32_t site,std::uint32_t code){status=invalid;diag(site,code);};
    auto* parser=s->create(c);
    if(!parser){status=oom;diag(0x20f48);}
    else{
        auto code=s->set(parser,0,input,0,c);
        const char* text=input;
        if(code==3){
            if((code=s->set(parser,1,"https",0,c)))fail(0x20ef0,code);
            else if((code=s->set(parser,5,input,0,c)))fail(0x20eb8,code);
            else if((code=s->get(parser,0,&rebuilt,0,c)))fail(0x20e78,code);
            // Original removes the last character unconditionally; refuse
            // a null/empty successful parser result rather than underflow.
            else if(!rebuilt||!*rebuilt)fail(0x20e78,3);
            else {rebuilt[std::strlen(rebuilt)-1]=0;text=rebuilt;}
        }else if(code)fail(0x20f20,code);
        if(!status){
            code=xcurl_research_wide_string(&wide,text,7,nullptr,nullptr,s->allocate,s->release,c);
            if(code){status=invalid;diag(0x20e50,reinterpret_cast<std::uintptr_t>(text),code);}
            else{
                alignas(8) std::array<unsigned char,0x68> parts{};
                status=xcurl_research_crack_url(wide,easy,parts.data(),s->crack);
                if(status&0x80000000U)diag(0x20e10,status);
            }
        }
        if(rebuilt)s->release(rebuilt,5,c);
        s->destroy(parser,c);
    }
    if(status&0x80000000U){if(wide)s->release(wide,7,c);wide=nullptr;}
    std::memcpy(out,&wide,sizeof(wide));
    if(!(status&0x80000000U))diag(0x20f60,reinterpret_cast<std::uintptr_t>(wide));
    return status;
}
