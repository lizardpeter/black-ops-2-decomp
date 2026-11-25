#include "upload_trailers.h"
#include "xcurl_leaf_api.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
using namespace t6_xcurl;
namespace {
template<class T>T get(void* p,unsigned o){T v;std::memcpy(&v,static_cast<char*>(p)+o,sizeof(v));return v;}
template<class T>void put(void* p,unsigned o,T v){std::memcpy(static_cast<char*>(p)+o,&v,sizeof(v));}
struct Host{unsigned allocs=0,frees=0,locks=0,diags=0,mode=0;bool fail=false;};
void* alloc(std::size_t n,std::uint32_t tag,void* c){auto& h=*static_cast<Host*>(c);assert(tag==16);if(h.fail)return nullptr;++h.allocs;return std::malloc(n);}
void free_buf(void* p,std::uint32_t tag,void* c){assert(tag==16&&p);++static_cast<Host*>(c)->frees;std::free(p);}
void enter(void*,void* c){++static_cast<Host*>(c)->locks;}
void leave(void*,void* c){auto& h=*static_cast<Host*>(c);assert(h.locks);--h.locks;}
void diag(void*,std::uint32_t,std::uint32_t status,void* c){assert(status&0x80000000U);++static_cast<Host*>(c)->diags;}
int callback(curl_slist** out,void* c){
    auto& h=*static_cast<Host*>(c);
    if(h.mode==1)return 0;
    *out=curl_slist_append(nullptr,"X-First: a");
    assert(*out&&curl_slist_append(*out,"X-Second: longer value"));
    return h.mode==2?1:0;
}
}
int main(){
    for(unsigned mode=0;mode<5;++mode){
        Host h;h.mode=mode==3||mode==4?0:mode;
        RequestServices s{};s.allocate=alloc;s.release=free_buf;s.enter=enter;s.leave=leave;s.diagnostic=diag;s.context=&h;
        alignas(8) std::array<char,0x368> easy{};auto* req=easy.data()+0x230;
        put(req,0x30,easy.data());put(easy.data(),0x138,&h);put(easy.data(),0x140,&callback);
        // Test both capacity reuse and actual buffer replacement.
        auto size=mode==0?128U:12U;auto* buffer=alloc(size,16,&h);
        put(req,0xb0,buffer);put(req,0xa8,static_cast<std::uint64_t>(size));
        h.fail=mode==4;std::uint64_t n=99;
        auto status=bound_append_upload_trailers(req,&n,&s);
        if(mode==0||mode==3){
            std::string expected="0\r\nX-First: a\r\nX-Second: longer value\r\n\r\n";
            assert(!status&&n==expected.size()&&std::string(get<char*>(req,0xb0),n)==expected);
            assert(h.allocs==(mode==0?1:2)&&h.frees==(mode==0?0:1)&&!h.diags);
        }else if(mode==1){assert(!status&&!n&&h.allocs==1&&!h.frees&&!h.diags);}
        else if(mode==2){assert(status==0x80004004U&&!n&&get<std::uint32_t>(easy.data(),0x220)==42&&h.diags==1);}
        else{assert(status==0x8007000eU&&!n&&!get<void*>(req,0xb0)&&get<std::uint32_t>(easy.data(),0x220)==27&&h.diags==1);}
        if(auto* p=get<void*>(req,0xb0))free_buf(p,16,&h);
        assert(!h.locks&&h.allocs==h.frees);
    }
}
