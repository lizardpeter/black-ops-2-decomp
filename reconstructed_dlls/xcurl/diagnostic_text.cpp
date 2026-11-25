#include "diagnostic_text.h"
#include <algorithm>
#include <array>
#include <cstring>
namespace t6_xcurl {
namespace {
template<class T>T field(const void* p,unsigned o){T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+o,sizeof(v));return v;}
}
void publish_error_text(void* e,const char* text,DebugData debug,void* c){
    if(!e||!text)return;
    auto* out=field<char*>(e,0x160);
    const bool verbose=field<unsigned char>(e,0x176)!=0;
    if(!out&&!verbose)return;
    std::array<char,257> buf{};
    auto n=std::min<std::size_t>(255,std::strlen(text));
    std::memcpy(buf.data(),text,n);
    if(out)std::memcpy(out,buf.data(),n+1);
    if(verbose&&debug){buf[n]='\n';debug(e,0,buf.data(),n+1,c);}
}
void publish_info_text(void* e,const char* text,bool newline,DebugData debug,void* c){
    if(!e||!text||!debug||!field<unsigned char>(e,0x176))return;
    std::array<char,2049> buf{};
    auto len=std::strlen(text);
    auto n=std::min<std::size_t>(2048,len);
    std::memcpy(buf.data(),text,n);
    if(len>2048){if(newline)std::memcpy(buf.data()+2044,"...\n",4);else std::memcpy(buf.data()+2045,"...",3);}
    debug(e,0,buf.data(),n,c);
}
}
