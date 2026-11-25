#include "url_object.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>
using namespace t6_xcurl;
struct Host{std::set<void*> live;unsigned calls=0,fail=0;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){
    auto& h=*static_cast<Host*>(c);assert(tag==5);
    if(++h.calls==h.fail)return nullptr;
    auto* p=std::malloc(n);assert(p);h.live.insert(p);std::memset(p,0xa5,n);return p;
}
void release(void* p,std::uint32_t tag,void* c){auto& h=*static_cast<Host*>(c);assert(tag==5&&h.live.erase(p)==1);std::free(p);}
char* dup(const char* str,void* c){auto n=std::strlen(str)+1;auto* p=static_cast<char*>(alloc(n,5,c));if(p)std::memcpy(p,str,n);return p;}
int main(){
    Host h;UrlObjectServices s{alloc,release,dup,&h};
    auto* u=static_cast<UrlObject*>(xcurl_research_url_create(&s));assert(u);
    for(auto* f:u->fields)assert(!f);assert(!u->port&&!u->padding);
    for(auto& f:u->fields)f=dup("owned",&h);
    u->port=443;u->padding=0xabcdef01;
    auto* copy=static_cast<UrlObject*>(xcurl_research_url_duplicate(u,&s));assert(copy&&copy->port==443&&!copy->padding);
    for(unsigned i=0;i<12;++i){
        if(i==5||i>=10)assert(!copy->fields[i]);else assert(copy->fields[i]!=u->fields[i]&&!std::strcmp(copy->fields[i],u->fields[i]));
    }
    xcurl_research_url_destroy(copy,&s);
    for(unsigned n=1;n<=10;++n){h.fail=h.calls+n;assert(!xcurl_research_url_duplicate(u,&s)&&h.live.size()==13);}
    h.fail=0;
    const unsigned slots[]={0,0,1,2,3,4,6,7,8,9,5};
    for(int part=1;part<=10;++part){
        char* out=nullptr;assert(!xcurl_research_url_get_part(u,part,&out,&s)&&out);
        assert(out!=u->fields[slots[part]]&&!std::strcmp(out,"owned"));release(out,5,&h);
        assert(!xcurl_research_url_clear_part(u,part,&s)&&!u->fields[slots[part]]);
    }
    assert(!u->port);
    char* out=nullptr;assert(!xcurl_research_url_get_part(u,7,&out,&s)&&!std::strcmp(out,"/")&&out!=u->fields[7]);release(out,5,&h);
    assert(xcurl_research_url_get_part(u,5,&out,&s)==14&&!out);
    assert(xcurl_research_url_get_part(u,10,&out,&s)==9);
    assert(xcurl_research_url_get_part(u,1,nullptr,&s)==2);
    auto* v=static_cast<UrlObject*>(xcurl_research_url_create(&s));v->fields[0]=dup("old",&h);
    xcurl_research_url_move(u,v,&s);assert(v->padding==0xabcdef01&&!std::strcmp(v->fields[7],"/"));
    xcurl_research_url_destroy(v,&s);assert(h.live.empty());
    char lower[42]{};assert(xcurl_research_url_has_scheme("HTTPS://example.test",lower)&&!std::strcmp(lower,"https"));
    assert(xcurl_research_url_has_scheme("x+y.z-1:/",nullptr));assert(!xcurl_research_url_has_scheme("C:/windows",nullptr));
    assert(!xcurl_research_url_has_scheme("https:foo",nullptr)&&!xcurl_research_url_has_scheme("a b://",nullptr));
}
