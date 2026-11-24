// Original Store XCurl 0x1AC88..0x1ADA3, disabled ETW.
// Retain the exact Windows conversion API on Windows; the portable host
// oracle uses strict UTF-8 decoding for the same CP_UTF8 / MB_ERR_INVALID_CHARS.
#include "wide_string.h"
#include <cstring>
#include <limits>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace {
int convert(const char* source,int len,std::uint16_t* out,int capacity) {
#ifdef _WIN32
    return ::MultiByteToWideChar(65001,8,source,len,reinterpret_cast<wchar_t*>(out),capacity);
#else
    if(len<=0)return 0;
    auto* p=reinterpret_cast<const unsigned char*>(source);
    int count=0;
    for(int i=0;i<len;) {
        unsigned c=p[i++],n=0,min=0;
        if(c<0x80){}
        else if(c>=0xc2&&c<=0xdf){c&=0x1f;n=1;min=0x80;}
        else if(c>=0xe0&&c<=0xef){c&=0xf;n=2;min=0x800;}
        else if(c>=0xf0&&c<=0xf4){c&=7;n=3;min=0x10000;}
        else return 0;
        if(n>static_cast<unsigned>(len-i))return 0;
        for(unsigned j=0;j<n;++j){unsigned b=p[i++];if((b&0xc0)!=0x80)return 0;c=(c<<6)|(b&0x3f);}
        if(c<min||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return 0;
        const int units=c>=0x10000?2:1;
        if(count>std::numeric_limits<int>::max()-units)return 0;
        if(out&&capacity>0){
            if(count+units>capacity)return 0;
            if(units==2){c-=0x10000;out[count]=static_cast<std::uint16_t>(0xd800+(c>>10));out[count+1]=static_cast<std::uint16_t>(0xdc00+(c&0x3ff));}
            else out[count]=static_cast<std::uint16_t>(c);
        }
        count+=units;
    }
    return count;
#endif
}
struct Owned {t6_xcurl::OptionAllocate allocate;t6_xcurl::OptionFree release;void* context;};
void* alloc_bridge(std::size_t n,std::uint32_t tag,void* ctx) {
    auto& s=*static_cast<Owned*>(ctx);return s.allocate(n,tag,s.context);
}
void free_bridge(void* p,std::uint32_t tag,void* ctx) {
    auto& s=*static_cast<Owned*>(ctx);s.release(p,tag,s.context);
}
std::uint32_t wide_bridge(void* slot,const char* text,std::uint32_t tag,void* ctx) {
    auto& s=*static_cast<Owned*>(ctx);return xcurl_research_wide_string(slot,text,tag,nullptr,nullptr,s.allocate,s.release,s.context);
}
}
extern "C" std::uint32_t xcurl_research_wide_string(void* slot,const char* text,std::uint32_t tag,
    std::uint64_t* capacity,std::uint64_t* written,t6_xcurl::OptionAllocate allocate,t6_xcurl::OptionFree release,void* context) {
    std::uint16_t* dest=nullptr;
    auto store=[&]{std::memcpy(slot,&dest,sizeof(dest));};
    std::memcpy(&dest,slot,sizeof(dest));
    if(!text){if(dest){release(dest,tag,context);dest=nullptr;store();}return 0;}
    const auto len=static_cast<int>(std::strlen(text));
    const auto units=convert(text,len,nullptr,0);
    if(!dest||!capacity||*capacity<static_cast<std::uint64_t>(units)){
        if(dest){release(dest,tag,context);dest=nullptr;store();}
        dest=static_cast<std::uint16_t*>(allocate(static_cast<std::size_t>(units)*2+2,tag,context));store();
        if(!dest)return 27;
        if(capacity)*capacity=static_cast<std::uint64_t>(units);
    }
    (void)convert(text,len,dest,units);
    dest[units]=0;
    if(written)*written=static_cast<std::uint64_t>(units);
    return 0;
}
extern "C" std::uint32_t xcurl_research_dispatch_owned_option(void* options,int option,const std::uint64_t* arg,
    t6_xcurl::OptionAllocate allocate,t6_xcurl::OptionFree release,void* context) {
    Owned owner{allocate,release,context};
    t6_xcurl::OptionServices services{alloc_bridge,free_bridge,wide_bridge,&owner};
    return xcurl_research_dispatch_option(options,option,arg,&services);
}
