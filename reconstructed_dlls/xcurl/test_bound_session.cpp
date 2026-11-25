#include "session_cache.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <set>
using namespace t6_xcurl;
struct Host{unsigned opens=0,closes=0,diagnostics=0,depth=0;std::uint32_t error=87;bool fail_open=false,retry=false,fail_option=false;unsigned fail_tag=0;std::set<void*> allocations;};
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<char*>(p)+o,&v,sizeof(v));}
void enter(void*,void* c){++static_cast<Host*>(c)->depth;}
void leave(void*,void* c){auto& h=*static_cast<Host*>(c);assert(h.depth);--h.depth;}
void* alloc(std::size_t n,std::uint32_t tag,void* c){auto& h=*static_cast<Host*>(c);if(tag==h.fail_tag)return nullptr;auto* p=std::malloc(n);assert(p);h.allocations.insert(p);return p;}
void release(void* p,std::uint32_t,void* c){assert(static_cast<Host*>(c)->allocations.erase(p)==1);std::free(p);}
void* open(const char16_t*,std::uint32_t flags,void* c){auto& h=*static_cast<Host*>(c);++h.opens;if(h.fail_open||(h.retry&&h.opens==1))return nullptr;if(h.retry)assert(flags==0x10000000U);return reinterpret_cast<void*>(0x1234);}
std::uint32_t option(void* p,std::uint32_t k,std::uint32_t n,void* c){assert(p==reinterpret_cast<void*>(0x1234)&&k==84&&n==0x800);return !static_cast<Host*>(c)->fail_option;}
std::uint32_t error(void* c){return static_cast<Host*>(c)->error;}
void close(void* p,void* c){assert(p==reinterpret_cast<void*>(0x1234));++static_cast<Host*>(c)->closes;}
void diag(void*,std::uint32_t,std::uint32_t status,void* c){assert(status&0x80000000U);++static_cast<Host*>(c)->diagnostics;}
int main(){
    for(unsigned mode=0;mode<9;++mode){
        Host h;h.retry=mode==1;h.fail_option=mode==2||mode==3;
        h.error=mode==3||mode==5?0:mode==2?5:87;
        h.fail_open=mode==4||mode==5;h.fail_tag=mode==6?19:mode==7?7:mode==8?17:0;
        SessionServices s{enter,leave,alloc,release,open,option,error,close,diag,nullptr,&h};
        alignas(8) std::array<char,0x80> cache{};alignas(8) std::array<char,0x368> easy{};
        auto* head=cache.data()+0x28;put(head,0,head);put(head,8,head);
        void* p=nullptr;auto status=xcurl_bound_session_acquire(cache.data(),easy.data(),0x800,2,&p,&s);
        assert(!h.depth);
        if(mode<2){
            assert(!status&&p&&h.opens==(mode?2:1)&&h.closes==0);
            void* second=nullptr;assert(!xcurl_bound_session_acquire(cache.data(),easy.data(),0x800,2,&second,&s)&&second==p);
            assert(h.opens==(mode?2:1));
            assert(xcurl_research_session_release(p,&s)==2);
            assert(xcurl_research_session_release(second,&s)==1);
            xcurl_research_session_cache_clear(cache.data(),&s);assert(h.closes==1);
        }else{
            assert(status&0x80000000U);assert(!p&&h.diagnostics==1);
            assert(h.closes==(h.fail_open?0:1));
            xcurl_research_session_cache_clear(cache.data(),&s);
        }
        assert(h.allocations.empty()&&!h.depth);
    }
}
