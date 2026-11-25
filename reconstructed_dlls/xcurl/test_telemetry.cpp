#include "telemetry_lifecycle.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <limits>
using namespace t6_xcurl;
struct H{std::map<void*,unsigned> live;std::vector<unsigned> freed;std::vector<std::u16string> batches;bool fail=false;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){auto& h=*static_cast<H*>(c);if(h.fail)return nullptr;auto* p=std::malloc(n);assert(p);h.live[p]=tag;std::memset(p,0xa5,n);return p;}
void release(void* p,std::uint32_t tag,void* c){auto& h=*static_cast<H*>(c);assert(h.live.at(p)==tag);h.live.erase(p);h.freed.push_back(tag);std::free(p);}
std::uint32_t tick(void*){return 0xfedcba98U;}
std::uint64_t tick64(void*){return 0x12345678fedcba98ULL;}
void publish(const char16_t* p,void* c){static_cast<H*>(c)->batches.emplace_back(p);}
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
    // Twenty-record grouping and final partial batch.
    assert(!xcurl_research_telemetry_create(&p,&s));head=static_cast<char*>(p)+0x28;
    auto append=[&](std::int32_t x,std::uint32_t y,std::uint64_t z){
        auto* n=alloc(0x20,2,&h);auto* last=get<void*>(head,8);
        put(n,0,head);put(n,8,last);put(last,0,n);put(head,8,n);
        put(n,0x10,x);put(n,0x14,y);put(n,0x18,z);
    };
    for(int i=0;i<23;++i)append(-1,2,3);
    TelemetryFlushServices fs{&s,tick64,publish,&h};
    xcurl_research_telemetry_flush(p,&fs);
    std::u16string expected;for(int i=0;i<20;++i)expected+=u"-1,2,3;";
    assert(h.batches.size()==2&&h.batches[0]==expected&&h.batches[1]==u"-1,2,3;-1,2,3;-1,2,3;");
    assert(h.live.size()==1&&get<void*>(head,0)==head&&get<void*>(head,8)==head&&get<std::uint64_t>(p,0x20)==tick64(nullptr));
    h.batches.clear();
    for(unsigned i=0;i<16;++i)append(INT32_MIN,UINT32_MAX,UINT64_MAX);
    xcurl_research_telemetry_flush(p,&fs);
    std::u16string record=u"-2147483648,4294967295,18446744073709551615;";
    expected.clear();for(unsigned i=0;i<12;++i)expected+=record;
    assert(h.batches.size()==2&&h.batches[0]==expected.substr(0,499));
    expected.clear();for(unsigned i=0;i<4;++i)expected+=record;
    assert(h.batches[1]==expected&&h.live.size()==1);
    h.batches.clear();xcurl_research_telemetry_flush(p,&fs);assert(h.batches.empty());
    xcurl_research_telemetry_destroy_local(p,&s);assert(h.live.empty());
    std::array<std::uint8_t,16> guid{};for(unsigned i=0;i<16;++i)guid[i]=static_cast<std::uint8_t>(i);
    char16_t text[37];xcurl_research_telemetry_format_guid(guid.data(),text);
    assert(std::u16string(text)==u"03020100-0504-0706-0809-0A0B0C0D0E0F");
}
