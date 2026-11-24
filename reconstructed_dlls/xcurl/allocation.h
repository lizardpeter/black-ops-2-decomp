#pragma once
#include <cstddef>
#include <cstdint>
namespace t6_xcurl {
// The default allocator dispatch slots in the pinned original are
// 0x28018 -> 0x9590 (allocate), 0x28008 -> 0x95C0 (free),
// 0x28010 -> 0x95F0 (reallocate). Explicit host heap boundary for tests.
using HeapAllocate=void*(*)(std::size_t,void*);
using HeapFree=void(*)(void*,void*);
using HeapReallocate=void*(*)(void*,std::size_t,void*);
}
extern "C" void xcurl_research_set_heap(t6_xcurl::HeapAllocate,t6_xcurl::HeapFree,
    t6_xcurl::HeapReallocate,void*);
extern "C" void* xcurl_research_heap_allocate(std::size_t bytes,std::uint32_t tag);
extern "C" void xcurl_research_heap_free(void* pointer,std::uint32_t tag);
extern "C" void* xcurl_research_heap_reallocate(void* pointer,std::size_t bytes,std::uint32_t tag);
