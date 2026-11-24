// XCurl Store x64, SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Original tagged dispatch 0x932C/0x9640 and default heap callbacks
// 0x9590/0x95C0/0x95F0. The tag is forwarded to original telemetry,
// not used by the default HeapAlloc/HeapFree operation. This implements
// the disabled-provider branch only; ETW and custom init_mem are separate.
#include "allocation.h"
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
namespace {
using namespace t6_xcurl;
void* default_allocate(std::size_t n,void*) {
#ifdef _WIN32
    return HeapAlloc(GetProcessHeap(),0,n);
#else
    return std::malloc(n);
#endif
}
void default_free(void* p,void*) {
#ifdef _WIN32
    ::HeapFree(GetProcessHeap(),0,p);
#else
    std::free(p);
#endif
}
void* default_reallocate(void* p,std::size_t n,void* ctx) {
    // Original 0x95F0 branches to HeapAlloc when the previous pointer is null.
    if(!p)return default_allocate(n,ctx);
#ifdef _WIN32
    return HeapReAlloc(GetProcessHeap(),0,p,n);
#else
    return std::realloc(p,n);
#endif
}
HeapAllocate allocate=default_allocate;
t6_xcurl::HeapFree release=default_free;
HeapReallocate reallocate=default_reallocate;
void* context=nullptr;
}
extern "C" void xcurl_research_set_heap(t6_xcurl::HeapAllocate a,t6_xcurl::HeapFree f,
    t6_xcurl::HeapReallocate r,void* ctx) {
    // Test-only boundary. Configure before allocating any owned objects.
    allocate=a?a:default_allocate;release=f?f:default_free;
    reallocate=r?r:default_reallocate;context=ctx;
}
namespace t6_xcurl::internal {
void* allocate_tagged(std::size_t n,std::uint32_t tag) {
    (void)tag;return allocate(n,context);
}
void free_tagged(void* p,std::uint32_t tag) {
    (void)tag;release(p,context);
}
void* copy_bytes(void* dst,const void* src,std::size_t n) {
    // Original 0x4AD0 is the byte-copy primitive. The duplicate_string
    // caller uses disjoint source/destination; memmove preserves overlap too.
    return std::memmove(dst,src,n);
}
}
extern "C" void* xcurl_research_heap_allocate(std::size_t n,std::uint32_t tag) {
    return t6_xcurl::internal::allocate_tagged(n,tag);
}
extern "C" void xcurl_research_heap_free(void* p,std::uint32_t tag) {
    t6_xcurl::internal::free_tagged(p,tag);
}
extern "C" void* xcurl_research_heap_reallocate(void* p,std::size_t n,std::uint32_t tag) {
    (void)tag;return p?reallocate(p,n,context):allocate(n,context);
}
