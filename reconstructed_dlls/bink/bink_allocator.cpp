// Original Store Bink x64 SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "bink_allocator.h"
#include <cstring>
#include <cstdint>
extern "C" void* bink_research_allocate_custom(std::uint32_t bytes,
    t6_bink::Allocate allocate,t6_bink::Deallocate free_fn) {
    if(!bytes || bytes==UINT32_MAX || !allocate || !free_fn)return nullptr;
    // 0x102D: original ECX arithmetic wraps in 32 bits.
    // Reject an overflowing request before handing a too-small block to
    // the game allocator rather than reproducing an invalid write.
    if(bytes>UINT32_MAX-64U)return nullptr;
    auto* raw=static_cast<std::uint8_t*>(allocate(bytes+64U));
    if(!raw || reinterpret_cast<std::uintptr_t>(raw)==UINTPTR_MAX)return nullptr;
    // Original does not simply align_up(raw,32). It leaves 33..64 bytes
    // before the user pointer, even when raw is already aligned.
    const auto delta=64U-(reinterpret_cast<std::uintptr_t>(raw)&31U);
    auto* user=raw+delta;
    user[-1]=static_cast<std::uint8_t>(delta);
    user[-2]=3;
    std::memcpy(user-12,&free_fn,sizeof(free_fn));
    return user;
}
extern "C" int bink_research_free_custom(void* memory) {
    if(!memory)return 0;
    auto* user=static_cast<std::uint8_t*>(memory);
    if(user[-2]!=3 || user[-1]<33 || user[-1]>64)return -1;
    t6_bink::Deallocate free_fn{};
    std::memcpy(&free_fn,user-12,sizeof(free_fn));
    if(!free_fn)return -1;
    free_fn(user-user[-1]);
    return 0;
}
