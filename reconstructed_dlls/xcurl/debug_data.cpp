// Original Store XCurl 0xA128, disabled ETW. Evidence run 45546001831.
// OS debug output and CRT stream writes are the explicit leaf boundaries.
#include "debug_data.h"
#include <array>
#include <cstring>
#include <algorithm>
namespace {
template<class T>T get(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const char*>(p)+o,sizeof(v));return v;}
using Callback=int(*)(void*,int,const void*,std::uint64_t,void*);
}
extern "C" int xcurl_research_debug_data(void* easy,int type,const void* data,std::uint64_t n,const t6_xcurl::DebugServices* s){
    if(!easy)return 0;
    if(auto cb=get<Callback>(easy,0x158))return cb(easy,type,data,n,get<void*>(easy,0x150));
    if(type<0||type>2)return 0;
    if(auto stream=get<void*>(easy,0x168)){
        constexpr const char* prefix[]={"* ","< ","> "};
        s->write(prefix[type],2,1,stream,s->context);
        s->write(data,n,1,stream,s->context);
    }else{
        // Original formats "* %s\n" into a 0x804-byte buffer even for
        // header types 1/2. Bound the read by n rather than trusting NUL
        // termination on an incoming network buffer (integration safety).
        std::array<char,0x804> message{};message[0]='*';message[1]=' ';
        std::size_t count=0;const auto limit=std::min<std::uint64_t>(n,message.size()-4);
        if(data){auto* p=static_cast<const char*>(data);while(count<limit&&p[count])++count;std::memcpy(message.data()+2,p,count);}
        message[count+2]='\n';s->output(message.data(),s->context);
    }
    return 0;
}
