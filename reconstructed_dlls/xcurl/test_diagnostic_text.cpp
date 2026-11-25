#include "diagnostic_text.h"
#include <array>
#include <cassert>
#include <cstring>
#include <string>
using namespace t6_xcurl;
namespace {
struct Trace{unsigned calls=0;std::string data;};
void debug(void*,std::uint32_t type,const void* data,std::uint64_t n,void* c){
    auto& t=*static_cast<Trace*>(c);assert(type==0);++t.calls;t.data.assign(static_cast<const char*>(data),n);
}
}
int main(){
    std::array<unsigned char,0x380> e{};std::array<char,258> buffer{};buffer.fill('!');
    char* p=buffer.data()+1;Trace t;
    publish_error_text(nullptr,"ignored",debug,&t);
    publish_error_text(e.data(),"ignored",debug,&t);assert(!t.calls);
    std::memcpy(e.data()+0x160,&p,sizeof(p));
    publish_error_text(e.data(),"failure",debug,&t);
    assert(std::string(p)=="failure"&&!t.calls);
    e[0x176]=1;
    publish_error_text(e.data(),"again",debug,&t);assert(t.data=="again\n"&&std::string(p)=="again");
    publish_info_text(e.data(),"informational",false,debug,&t);assert(t.data=="informational"&&std::string(p)=="again");
    std::string long_text(4000,'x');
    publish_error_text(e.data(),long_text.c_str(),debug,&t);
    assert(std::strlen(p)==255&&t.data.size()==256&&t.data.back()=='\n'&&buffer.front()=='!'&&buffer.back()=='!');
    publish_info_text(e.data(),long_text.c_str(),false,debug,&t);assert(t.data.size()==2048&&t.data.substr(2045)=="...");
    publish_info_text(e.data(),long_text.c_str(),true,debug,&t);assert(t.data.size()==2048&&t.data.substr(2044)=="...\n");
    p=nullptr;std::memcpy(e.data()+0x160,&p,sizeof(p));publish_error_text(e.data(),"verbose only",debug,&t);assert(t.data=="verbose only\n");
}
