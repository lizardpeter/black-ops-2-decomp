#include "debug_data.h"
#include <array>
#include <cassert>
#include <cstring>
#include <string>
using namespace t6_xcurl;
struct H{std::string output;std::string bytes;int calls=0;void* stream=nullptr;};
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<char*>(p)+o,&v,sizeof(v));}
void output(const char* p,void* c){static_cast<H*>(c)->output+=p;}
void write(const void* p,std::uint64_t n,std::uint64_t k,void* stream,void* c){auto& h=*static_cast<H*>(c);assert(stream==h.stream&&k==1);h.bytes.append(static_cast<const char*>(p),n);}
int cb(void*,int type,const void* p,std::uint64_t n,void* c){auto& h=*static_cast<H*>(c);assert(type==4&&n==3&&!std::memcmp(p,"abc",3));++h.calls;return 17;}
int main(){
    H h;DebugServices s{output,write,&h};std::array<char,0x368> e{};
    assert(!xcurl_research_debug_data(nullptr,0,"x",1,&s));
    const char input[]={'a','b','c'};
    for(int type=0;type<3;++type)assert(!xcurl_research_debug_data(e.data(),type,input,3,&s));
    assert(h.output=="* abc\n* abc\n* abc\n");
    assert(!xcurl_research_debug_data(e.data(),3,input,3,&s)&&h.output.size()==18);
    h.stream=&h;put(e.data(),0x168,h.stream);
    for(int type=0;type<3;++type)xcurl_research_debug_data(e.data(),type,input,3,&s);
    assert(h.bytes=="* abc< abc> abc");
    put(e.data(),0x150,&h);put(e.data(),0x158,&cb);
    assert(xcurl_research_debug_data(e.data(),4,input,3,&s)==17&&h.calls==1);
}
