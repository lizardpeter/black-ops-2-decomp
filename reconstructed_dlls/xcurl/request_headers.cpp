// Original 0x10038, disabled ETW. The list nodes are original curl_slist layout.
#include "request_headers.h"
#include "wide_string.h"
#include "http_setup.h"
#include <bit>
#include <cstring>
#include <string_view>
using namespace t6_xcurl;
namespace {
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const std::uint8_t*>(p)+o,sizeof(v));return v;}
std::uint32_t hr(std::uint32_t x){return std::bit_cast<std::int32_t>(x)>0?(x&0xffffU)|0x80070000U:x;}
}
extern "C" std::uint32_t xcurl_research_add_request_headers(void* request,const void* list,const AddHeaderServices* a){
    auto* s=a->request;auto c=s->context;void* wide=nullptr;std::uint64_t capacity=0,written=0;std::uint32_t result=0;
    for(auto* node=list;node;node=get<const void*>(node,8)){
        auto* text=get<const char*>(node,0);auto status=xcurl_research_wide_string(&wide,text,8,&capacity,&written,s->allocate,s->release,c);
        if(status){result=status==27?0x8007000eU:0x80004005U;break;}
        auto* e=get<std::uint8_t*>(request,0x30);auto mode=get<std::uint32_t>(request,0xd0);
        if((mode==0||mode==1)&&(get<std::uint32_t>(e,0x80)!=2||!get<void*>(e,0xc0))&&written>=17&&wide){
            auto view=std::u16string_view(static_cast<const char16_t*>(wide),written);
            if(view.starts_with(u"Transfer-Encoding")&&view.substr(17).find(u"chunked")!=std::u16string_view::npos)xcurl_research_upload_mode(request,2);
        }
        if(e[0x176]&&text)s->debug_data(e,2,text,std::strlen(text),c);
        result=0;if(!a->add(get<void*>(request,0x50),static_cast<const char16_t*>(wide),static_cast<std::uint32_t>(written),0xa0000000U,c)){
            result=hr(s->last_error(c));if(result==0x80072f76U)result=0;else break;
        }
    }
    if(wide)s->release(wide,8,c);
    if(result&0x80000000U)s->diagnostic(get<void*>(request,0x30),0x211a0,result,c);
    return result;
}
