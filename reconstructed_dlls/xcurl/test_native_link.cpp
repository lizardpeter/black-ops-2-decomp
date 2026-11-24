#include "xcurl_leaf_api.h"
#include "allocation.h"
#include "global_lifecycle.h"
#include <array>
#include <cassert>
#include <cstring>
using namespace t6_xcurl;
namespace {
struct Context {unsigned enters=0,leaves=0,inits=0,destroys=0,frees=0,shutdowns=0,errors=0;};
void enter(void*,void* c){++static_cast<Context*>(c)->enters;}
void leave(void*,void* c){++static_cast<Context*>(c)->leaves;}
std::uint64_t tick(void*){return 123;}
std::uint32_t init(GlobalLifecycle* s,void* c){++static_cast<Context*>(c)->inits;s->engine=c;return 0;}
void destroy(void* p,void* c){assert(p==c);++static_cast<Context*>(c)->destroys;}
void release(void* p,std::uint32_t tag,void* c){assert(p==c&&tag==11);++static_cast<Context*>(c)->frees;}
void shutdown(void* c){++static_cast<Context*>(c)->shutdowns;}
void error(std::uint32_t,std::uint32_t,void* c){++static_cast<Context*>(c)->errors;}
}
int main(){
    // Uses real reconstructed list, allocator and copy source together.
    char input[]="alpha";
    auto* list=curl_slist_append(nullptr,input);
    assert(list&&list->data!=input&&!list->next);
    input[0]='X';
    assert(std::strcmp(list->data,"alpha")==0);
    assert(curl_slist_append(list,"beta")==list);
    assert(list->next&&!list->next->next&&std::strcmp(list->next->data,"beta")==0);
    curl_slist_free_all(list);
    curl_slist_free_all(nullptr);
    assert(std::strcmp(curl_easy_strerror(0),"No error")==0);
    assert(std::strcmp(curl_easy_strerror(-1),"Unknown error")==0);
    auto* p=static_cast<unsigned char*>(xcurl_research_heap_reallocate(nullptr,8,5));
    assert(p);std::memset(p,0x5a,8);
    auto* q=static_cast<unsigned char*>(xcurl_research_heap_reallocate(p,32,5));
    assert(q);for(unsigned i=0;i<8;++i)assert(q[i]==0x5a);
    xcurl_research_heap_free(q,5);
    Context c;ApiProfile profile;GlobalLifecycle global;
    for(int i=0;i<2;++i)assert(xcurl_research_global_init(&global,&profile,&c,enter,leave,tick,init,error,&c)==0);
    assert(global.references==2&&global.engine==&c&&c.inits==1);
    std::array<unsigned char,0x240> handle{};
    std::uint32_t expected=204,actual=0;
    std::memcpy(handle.data()+0x1e0,&expected,sizeof(expected));
    assert(xcurl_research_easy_getinfo(handle.data(),0x200002,&actual,&profile,&c,enter,leave,tick,error,&c)==0);
    assert(actual==expected);
    assert(xcurl_research_easy_getinfo(nullptr,0x200002,&actual,&profile,&c,enter,leave,tick,error,&c)==0x30);
    for(int i=0;i<3;++i)xcurl_research_global_cleanup(&global,&profile,&c,enter,leave,tick,destroy,release,shutdown,&c);
    assert(global.references==0&&!global.engine&&c.destroys==1&&c.frees==1&&c.shutdowns==1);
    assert(profile.first_tick==123&&profile.counts[0]==2&&profile.counts[3]==3&&profile.counts[10]==2);
    assert(c.enters==8&&c.leaves==8&&c.errors==1);
}
