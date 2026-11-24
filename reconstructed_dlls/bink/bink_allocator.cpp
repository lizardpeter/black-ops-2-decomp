// Original Store Bink x64 SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "bink_allocator.h"
#include <cstring>
#include <cstdint>
extern "C" void* bink_research_allocate_with_fallback(std::uint32_t bytes,
    t6_bink::Allocate allocate,t6_bink::Deallocate free_fn,t6_bink::Allocate fallback) {
    if(!bytes || bytes==UINT32_MAX || bytes>UINT32_MAX-64U)return nullptr;
    auto* raw=allocate?static_cast<std::uint8_t*>(allocate(bytes+64U)):nullptr;
    if(reinterpret_cast<std::uintptr_t>(raw)==UINTPTR_MAX)return nullptr;
    // Original custom-null (but not -1) falls through to the default heap.
    const bool custom=raw!=nullptr;
    if(!raw && fallback)raw=static_cast<std::uint8_t*>(fallback(bytes+64U));
    if(!raw)return nullptr;
    const auto delta=64U-(reinterpret_cast<std::uintptr_t>(raw)&31U);
    auto* user=raw+delta;
    user[-1]=static_cast<std::uint8_t>(delta);
    user[-2]=custom?3:0;
    if(custom)std::memcpy(user-12,&free_fn,sizeof(free_fn));
    return user;
}
extern "C" int bink_research_free_with_fallback(void* memory,t6_bink::Deallocate fallback_free) {
    if(!memory)return 0;
    auto* user=static_cast<std::uint8_t*>(memory);
    if(user[-1]<33 || user[-1]>64)return -1;
    auto free_fn=fallback_free;
    if(user[-2]==3)std::memcpy(&free_fn,user-12,sizeof(free_fn));
    if(!free_fn)return -1;
    free_fn(user-user[-1]);
    return 0;
}
extern "C" void* bink_research_allocate_custom(std::uint32_t bytes,
    t6_bink::Allocate allocate,t6_bink::Deallocate free_fn) {
    return bink_research_allocate_with_fallback(bytes,allocate,free_fn,nullptr);
}
extern "C" int bink_research_free_custom(void* memory) {
    return bink_research_free_with_fallback(memory,nullptr);
}
