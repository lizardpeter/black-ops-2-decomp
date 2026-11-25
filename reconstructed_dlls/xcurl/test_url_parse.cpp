#include "url_parse.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
using namespace t6_xcurl;
struct H{std::set<void*> live;unsigned calls=0,fail=0;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){auto& h=*static_cast<H*>(c);assert(tag==5);if(++h.calls==h.fail)return nullptr;auto* p=std::malloc(n);assert(p);h.live.insert(p);return p;}
void release(void* p,std::uint32_t tag,void* c){assert(tag==5&&static_cast<H*>(c)->live.erase(p)==1);std::free(p);}
char* dup(const char* p,void* c){auto n=std::strlen(p)+1;auto* out=static_cast<char*>(alloc(n,5,c));if(out)std::memcpy(out,p,n);return out;}
int main(){
    H h;UrlObjectServices s{alloc,release,dup,&h};
    auto* u=static_cast<UrlObject*>(xcurl_research_url_create(&s));
    auto set=[&](const char* text){return xcurl_research_url_set_http(u,0,text,&s);};
    auto format=[&]{char* p=nullptr;assert(!xcurl_research_url_format(u,&p,0,&s));std::string value=p;release(p,5,&h);return value;};
    assert(!set("HTTPS://example.test")&&format()=="https://example.test/");
    assert(!set("https://user:pass@example.test:00443/a/./b/../c?q=1#frag")&&format()=="https://user:pass@example.test:443/a/c?q=1#frag");
    assert(!set("http://[fe80::1%25eth0]:8080/a")&&format()=="http://[fe80::1%25eth0]:8080/a");
    assert(!set("http:///example.test/a")&&format()=="http://example.test/a");
    assert(set("http:////example.test")==3&&format()=="http://example.test/a");
    assert(set("http://host:0")==4&&set("http://host:65536")==4&&set("http://bad host")==3);
    assert(set("http://host/a\nb")==3&&set("ftp://host/")==5&&set("example.test")==3);
    assert(format()=="http://example.test/a");
    auto before=h.live.size();
    for(unsigned k=1;k<=12;++k){h.fail=h.calls+k;auto st=set("https://u:p@example.test:443/a/../b?q=x#z");h.fail=0;
        if(st)assert(h.live.size()==before);else {assert(format()=="https://u:p@example.test:443/b?q=x#z");before=h.live.size();}
    }
    xcurl_research_url_destroy(u,&s);assert(h.live.empty());
    const char* paths[]={"/a/./b","/a/../b","/a/..","/a/.","/../x","a/../../b","/a/../b?x=/../z"};
    const char* expected[]={"/a/b","/b","/","/a/","/x","/b","/b?x=/../z"};
    for(unsigned i=0;i<7;++i){auto* p=xcurl_research_url_normalize_path(paths[i],&s);assert(p&&!std::strcmp(p,expected[i]));release(p,5,&h);}
    assert(h.live.empty());
}
