#pragma once
#include <cstddef>
#include <cstdint>
namespace t6_bink {
using Allocate=void* (*)(std::size_t);
using Deallocate=void (*)(void*);
}
// Original custom allocator branch at 0x1010 and deallocator at 0x10D0.
// These helpers preserve the 64-byte overhead and original footer. A
// caller owns the raw allocation; the footer retains the free callback
// that was active when that allocation was made.
extern "C" void* bink_research_allocate_custom(std::uint32_t bytes,
    t6_bink::Allocate allocate,t6_bink::Deallocate free_fn);
extern "C" int bink_research_free_custom(void* memory);
