// XCurl Store x64 SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Directly transcribed from local disassembly of 0xEAF8..0xFCB3.
// Original telemetry disabled; the Windows conversion callee 0x1AC88
// remains an explicit boundary. Options are the original packed layout,
// not libcurl's CURL object. Caller owns synchronization and valid input.
#include "option_dispatch.h"
#include <algorithm>
#include <bit>
#include <cstring>
using namespace t6_xcurl;
namespace {
template<class T> T load(const std::uint8_t* p,unsigned o) {
    T v;std::memcpy(&v,p+o,sizeof(v));return v;
}
template<class T> void put(std::uint8_t* p,unsigned o,T v) {
    std::memcpy(p+o,&v,sizeof(v));
}
}
extern "C" std::uint32_t xcurl_research_dispatch_option(void* options,int option,
    const std::uint64_t* arg,const OptionServices* s) {
    auto* p=static_cast<std::uint8_t*>(options);
    const auto q=*arg;
    const auto u=static_cast<std::uint32_t>(q);
    const auto n=std::bit_cast<std::int32_t>(u);
    const auto n64=std::bit_cast<std::int64_t>(q);
    const auto str=reinterpret_cast<const char*>(static_cast<std::uintptr_t>(q));
    auto pointer=[&](unsigned off){put(p,off,q);return 0U;};
    auto flag=[&](unsigned off,bool invert=false){put(p,off,std::uint8_t((u!=0)!=invert));return 0U;};
    auto method=[&](std::uint32_t m){if(u)put(p,8,m);return 0U;};
    auto free_at=[&](unsigned off) {
        if(auto* old=load<void*>(p,off)) {
            s->release(old,7,s->context);put<void*>(p,off,nullptr);
        }
    };
    auto size=[&](std::int64_t v) {
        if(v< -1)return 43U;
        if(load<std::int64_t>(p,0x80)<v&&p[0x100]==1)free_at(0x48);
        put(p,0x80,v);return 0U;
    };
    auto wide=[&](unsigned off){return s->wide_string(p+off,str,7,s->context);};
    switch(option) {
    case 3: // 0xEE32
        if(u>65535)return 43;
        put(p,0x18,std::uint16_t(u));return 0;
    case 13: case 78: // 0xEDF0, 0xEF41
        if(u>0x20c49b)return 43;
        put(p,option==13?0x28:0x2c,u*1000U);return 0;
    case 14: // 0xEDAB: writes even on invalid negative input
        put(p,0x78,std::int64_t(n));return n< -1?43U:0U;
    case 41:return flag(0xfe);
    case 42:return flag(0xfd);
    case 44:return method(3);
    case 45:return flag(0xff);
    case 46:case 54:return method(1);
    case 47:return method(2);
    case 52:return flag(0xfb);
    case 60:return size(n);
    case 64:return flag(0x101);
    case 68:
        if(n< -1)return 43;
        put(p,0x30,u);return 0;
    case 80:return method(0);
    case 81:return flag(0x102);
    case 84:
        if(u<=4){put(p,0x108,u);return 0;}
        return u==5||u==30?4U:43U;
    case 98:
        put(p,0x34,n<1||n>0x80000?0x80000U:std::max(u,0x2000U));return 0;
    case 136:return flag(0xfc);
    case 155:case 156:
        if(n<0)return 43;
        put(p,option==155?0x28:0x2c,u);return 0;
    case 157:return flag(0xf9,true);
    case 158:return flag(0xfa,true);
    case 161:
        if(n<0)return 43;
        put(p,0x104,u&7U);return 0;
    case 207:return flag(0xf8);
    case 280:
        put(p,0x38,std::clamp(n,0x2000,0x200000));return 0;
    case 10001:return pointer(0x90);
    case 10002: { // 0xF440: URL is an owned narrow copy, not UTF-16.
        // Original measures the string before releasing the old allocation.
        const auto len=str?std::strlen(str):0;
        free_at(0x10);
        if(!str)return 0;
        auto* dest=static_cast<char*>(s->allocate(len+1,7,s->context));
        put(p,0x10,dest);
        if(!dest)return 27;
        std::memmove(dest,str,len);dest[len]=0;return 0;
    }
    case 10009:return pointer(0xa0);
    case 10010:return pointer(0xe8);
    case 10015: // 0xF665: borrowed payload, clear owned flag
        if(p[0x100]==1)free_at(0x48);
        p[0x100]=0;put(p,0x48,q);put(p,8,2U);return 0;
    case 10016:return wide(0x70);
    case 10018:return wide(0x60);
    case 10023:return pointer(0x20);
    case 10025:return wide(0x58);
    case 10029:return pointer(0xb0);
    case 10036:return wide(0x50);
    case 10037:return pointer(0xf0);
    case 10095:return pointer(0xd8);
    case 10102: { // 0xF7BB: case-sensitive substring flags.
        // Null is not a valid original input on this branch; caller contract.
        if(!*str){put(p,0x68,0U);return 0;}
        auto flags=load<std::uint32_t>(p,0x68);
        if(std::strstr(str,"gzip"))flags|=1;
        if(std::strstr(str,"deflate"))flags|=2;
        if(std::strstr(str,"*"))flags=3;
        put(p,0x68,flags);return 0;
    }
    case 10103:return pointer(0xd0);
    case 10165: { // 0xF919: copied payload
        if(p[0x100]==1)free_at(0x48);
        auto len=load<std::int64_t>(p,0x80);
        std::uint32_t result=0;
        if(std::uint64_t(len)+1U>0x100000000ULL)result=27;
        else {
            if(len==-1)len=static_cast<std::int64_t>(std::strlen(str));
            if(len>0) {
                auto* dest=static_cast<char*>(s->allocate(static_cast<std::size_t>(len)+1,7,s->context));
                if(dest){std::memmove(dest,str,static_cast<std::size_t>(len));dest[len]=0;put(p,0x48,dest);}
                else result=27;
            }
            put(p,0x80,len);
        }
        p[0x100]=1;put(p,8,2U);return result;
    }
    case 10282:return pointer(0);
    case 10284:return pointer(0xc0);
    case 20011:return pointer(0x98);
    case 20012:return pointer(0xa8);
    case 20079:return pointer(0xb8);
    case 20094:return pointer(0xe0);
    case 20283:return pointer(0xc8);
    case 30115:
        if(n64< -1)return 43;
        put(p,0x78,n64);return 0;
    case 30120:return size(n64);
    case 172:case 232:case 233:case 286:case 10024:case 10097:case 10109:
    case 10230:case 10240:case 10241:case 10269:case 10287:case 20108:
        return 4;
    default:return 48;
    }
}
