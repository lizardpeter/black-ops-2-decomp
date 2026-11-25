#include "url_component.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string>
#include <set>
using namespace t6_xcurl;
struct H{std::set<void*> live;bool fail=false;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){assert(tag==5);auto& h=*static_cast<H*>(c);if(h.fail)return nullptr;auto p=std::malloc(n);assert(p);h.live.insert(p);return p;}
void release(void* p,std::uint32_t tag,void* c){assert(tag==5&&static_cast<H*>(c)->live.erase(p)==1);std::free(p);}
char* dup(const char* p,void* c){auto n=std::strlen(p)+1;auto* out=static_cast<char*>(alloc(n,5,c));if(out)std::memcpy(out,p,n);return out;}
int main(){
    H h;UrlObjectServices s{alloc,release,dup,&h};
    auto* u=static_cast<UrlObject*>(xcurl_research_url_create(&s));
    auto set=[&](int part,const char* p){return xcurl_research_url_set_part(u,part,p,&s);};
    auto get=[&](int part,unsigned f){char* p=nullptr;assert(!xcurl_research_url_get_flags(u,part,&p,f,&s)&&p);std::string result=p;release(p,5,&h);return result;};
    char* out=nullptr;assert(xcurl_research_url_format(u,&out,0,&s)==14);
    assert(set(1,"ftp")==5&&set(1,"HTTPS")==0);
    assert(set(5,"example.test")==0&&get(0,0)=="HTTPS://example.test/");
    assert(get(6,1)=="443"&&get(0,1)=="HTTPS://example.test:443/");
    assert(set(6,"00443")==0&&u->port==443&&get(6,0)=="00443");
    assert(get(0,2)=="HTTPS://example.test/");
    assert(set(6,"0")==4&&set(6,"65536")==4&&set(6,"80x")==3&&u->port==443);
    assert(set(7,"path/%AF")==0&&get(7,0)=="path/%af");
    assert(set(8,"a=1")==0&&set(9,"fragment")==0);
    assert(set(2,"user")==0&&set(3,"pass")==0&&set(4,"options")==0);
    assert(get(0,0)=="HTTPS://user:pass@example.test:00443/path/%af?a=1#fragment");
    assert(set(5,"bad host")==3&&get(5,0)=="example.test");
    assert(set(5,"[fe80::1%25eth0]")==0&&get(5,0)=="[fe80::1]"&&get(10,0)=="eth0");
    assert(get(0,0)=="HTTPS://user:pass@[fe80::1%25eth0]:00443/path/%af?a=1#fragment");
    assert(set(5,"[::1]")==0&&!u->fields[5]);
    h.fail=true;assert(set(7,"new")==7);h.fail=false;assert(get(7,0)=="path/%af");
    assert(set(1,nullptr)==0&&get(0,4)=="https://user:pass@[::1]:00443/path/%af?a=1#fragment");
    xcurl_research_url_destroy(u,&s);assert(h.live.empty());
}
