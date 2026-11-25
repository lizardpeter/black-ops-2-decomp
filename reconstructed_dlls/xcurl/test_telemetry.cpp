#include "telemetry_lifecycle.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>
using namespace t6_xcurl;
struct H{std::map<void*,unsigned> live;std::vector<unsigned> freed;bool fail=false;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){auto& h=*static_cast<H*>(c);if(h.fail)return nullptr;auto* p=std::malloc(n);assert(p);h.live[p]=tag;std::memset(p,0xa5,n);return p;}
void release(void* p,std::uint32_t tag,void* c){auto& h=*static_cast<H*>(c);assert(h.live.at(p)==tag);h.live.erase(p);h.freed.push_back(tag);std::free(p);}
std::uint32_t tick(void*){return 0xfedcba98U;}
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<char*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<char*>(p)+o,&v,sizeof(v));}
int main(){
    H h;TelemetryServices s{alloc,release,tick,&h};void* p=nullptr;
    h.fail=true;assert(xcurl_research_telemetry_create(&p,&s)==0x8007000eU&&!p);h.fail=false;
    assert(!xcurl_research_telemetry_create(&p,&s)&&p);
    auto* head=static_cast<char*>(p)+0x28;for(unsigned off=0;off<0x28;off+=8)assert(!get<std::uint64_t>(p,off));
    assert(get<void*>(head,0)==head&&get<void*>(head,8)==head&&get<std::uint64_t>(p,0x38)==0xfedcba98U);
    auto* a=alloc(0x20,2,&h);auto* b=alloc(0x20,2,&h);
    put(head,0,a);put(head,8,b);put(a,0,b);put(a,8,head);put(b,0,head);put(b,8,a);
    xcurl_research_telemetry_unlink(a);assert(get<void*>(a,0)==a&&get<void*>(a,8)==a&&get<void*>(head,0)==b&&get<void*>(b,8)==head);
    release(a,2,&h);xcurl_research_telemetry_destroy_local(p,&s);
    assert(h.live.empty()&&(h.freed==std::vector<unsigned>{2,2,3}));
    std::array<std::uint8_t,16> guid{};for(unsigned i=0;i<16;++i)guid[i]=static_cast<std::uint8_t>(i);
    char16_t text[37];xcurl_research_telemetry_format_guid(guid.data(),text);
    assert(std::u16string(text)==u"03020100-0504-0706-0809-0A0B0C0D0E0F");
}
