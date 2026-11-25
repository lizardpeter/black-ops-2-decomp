#include "url_prepare.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace t6_xcurl;
struct H{
    unsigned creates=0,destroys=0,allocs=0,frees=0,cracks=0,sets=0,gets=0;
    unsigned mode=0;std::uint32_t site=0;
    std::u16string text;
};
H& h(void* c){return *static_cast<H*>(c);}
void* create(void* c){++h(c).creates;return h(c).mode==1?nullptr:c;}
void destroy(void* p,void* c){assert(p==c);++h(c).destroys;}
void* alloc(std::size_t n,std::uint32_t,void* c){if(h(c).mode==7)return nullptr;++h(c).allocs;return std::malloc(n);}
void release(void* p,std::uint32_t,void* c){assert(p);++h(c).frees;std::free(p);}
std::uint32_t set(void* p,int part,const char* str,unsigned flags,void* c){
    assert(p==c&&flags==0);auto& s=h(c);++s.sets;
    if(part==0)return s.mode==0||s.mode==7||s.mode==8?0:s.mode==2?4:3;
    if(part==1){assert(std::strcmp(str,"https")==0);return s.mode==3?6:0;}
    assert(part==5);return s.mode==4?7:0;
}
std::uint32_t get(void* p,int part,char** out,unsigned flags,void* c){
    assert(p==c&&!part&&!flags);auto& s=h(c);++s.gets;if(s.mode==5)return 8;
    const char* text=s.mode==9?"":"https://example.test/";
    *out=static_cast<char*>(alloc(std::strlen(text)+1,5,c));std::strcpy(*out,text);return 0;
}
std::uint32_t crack(const char16_t* text,void* parts,void* c){
    auto& s=h(c);++s.cracks;s.text=text;std::uint32_t scheme=2;
    std::memcpy(static_cast<char*>(parts)+0x14,&scheme,4);return s.mode!=8;
}
std::uint32_t error(void*){return 12005;}
void crack_diag(void*,std::uint32_t,const char16_t*,std::uint32_t,void*){}
void diag(void*,std::uint32_t site,std::uintptr_t,std::uint32_t,void* c){h(c).site=site;}
int main(){
    const std::uint32_t sites[]={0x20f60,0x20f48,0x20f20,0x20ef0,0x20eb8,0x20e78,0x20f60,0x20e50,0x20e10,0x20e78};
    for(unsigned mode=0;mode<10;++mode){
        H s;s.mode=mode;UrlServices cs{crack,error,crack_diag,nullptr,nullptr,release,&s};
        UrlPreparationServices ps{create,set,get,destroy,alloc,release,diag,&cs,&s};
        char16_t* out=reinterpret_cast<char16_t*>(1);
        auto result=xcurl_research_prepare_url("example.test",nullptr,&out,&ps);
        bool ok=mode==0||mode==6;assert((result==0)==ok);assert(s.site==sites[mode]);
        assert(s.creates==1&&s.destroys==(mode!=1));assert((out!=nullptr)==ok);
        if(ok){assert(s.text==(mode==0?u"example.test":u"https://example.test"));release(out,7,&s);}
        assert(s.allocs==s.frees);
        if(mode==6)assert(s.sets==3&&s.gets==1&&s.cracks==1);
        if(mode==1)assert(result==0x8007000eU);
        if(mode==8)assert(result==0x80072ee5U);
    }
}
