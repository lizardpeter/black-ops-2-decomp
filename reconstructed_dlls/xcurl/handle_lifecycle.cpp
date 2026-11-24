// Original Store XCurl SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Disabled-ETW control flow for 0xDC94 / 0xDBC0, 0x199A8 / 0x19818,
// easy constructor 0xD848 (on zeroed storage), option destructor 0xE818.
#include "handle_lifecycle.h"
#include <atomic>
#include <cstring>
#include <initializer_list>
using namespace t6_xcurl;
namespace {
template<class T> void put(std::uint8_t* p,unsigned o,T v){std::memcpy(p+o,&v,sizeof(v));}
void self_list(std::uint8_t* p,unsigned o){put(p,o,p+o);put(p,o+8,p+o);}
void easy_construct(std::uint8_t* p,const HandleServices& s){
    // Storage was zeroed by 0xDC94, including all otherwise explicit zero stores.
    self_list(p,0);
    s.initialize_lock(p+0x10,s.context);
    s.initialize_lock(p+0x50,s.context);
    put(p,0xa4,0x493e0U);put(p,0xa8,10U);put(p,0xac,0x2000U);put(p,0xb0,0x10000U);
    put(p,0xf0,UINT64_MAX);put(p,0xf8,UINT64_MAX);
    put(p,0x170,std::uint64_t(0x10100));put(p,0x178,std::uint16_t(0x100));put(p,0x17a,std::uint8_t(1));
    s.initialize_lock(p+0x188,s.context);
    put(p,0x1f8,UINT64_MAX);
    s.initialize_lock(p+0x230,s.context);
    put(p,0x260,p);
}
}
extern "C" std::uint32_t xcurl_research_easy_create(void** out,std::uint32_t* refs,const HandleServices* s){
    // Original increments the global count before initialization and does
    // not roll it back if initialization fails. Preserve that behavior.
    std::atomic_ref<std::uint32_t> ref(*refs);
    if(ref.load(std::memory_order_relaxed)==0){
        ref.fetch_add(1,std::memory_order_relaxed);
        const auto status=s->start_global(s->context);if(status)return status;
    }
    auto* p=static_cast<std::uint8_t*>(s->allocate(0x368,6,s->context));*out=p;
    if(!p)return 27;
    std::memset(p,0,0x368);easy_construct(p,*s);
    if(s->start_easy(p,s->context)&0x80000000U){
        s->destroy_easy(p,s->context);s->release(p,6,s->context);*out=nullptr;return 2;
    }
    return 0;
}
extern "C" void xcurl_research_easy_destroy(void* p,const HandleServices* s){
    if(p){s->destroy_easy(p,s->context);s->release(p,6,s->context);}
}
extern "C" std::uint32_t xcurl_research_multi_create(void** out,const HandleServices* s){
    auto* p=static_cast<std::uint8_t*>(s->allocate(0xc8,13,s->context));*out=p;
    if(!p)return 3;
    std::memset(p+0x10,0,0xb8);self_list(p,0);s->initialize_lock(p+0x10,s->context);
    for(unsigned off:{0x40U,0x50U,0x60U,0x78U})self_list(p,off);
    put(p,0x94,std::uint64_t(100));
    const auto status=s->start_multi(p,s->context);
    if(status){s->release(p,13,s->context);*out=nullptr;}
    return status;
}
extern "C" std::uint32_t xcurl_research_multi_destroy(void* p,const HandleServices* s){
    if(!p)return 1;
    const auto status=s->destroy_multi(p,s->context);s->release(p,13,s->context);return status;
}
extern "C" void xcurl_research_options_destroy(void* options,OptionFree release,void* context){
    auto* p=static_cast<std::uint8_t*>(options);
    auto free_at=[&](unsigned o){void* v;std::memcpy(&v,p+o,sizeof(v));if(v){release(v,7,context);put<void*>(p,o,nullptr);}};
    free_at(0x10);if(p[0x100]==1)free_at(0x48);free_at(0x50);free_at(0x60);free_at(0x70);
}
