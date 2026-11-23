// Reconstructed XCurl internal string duplicator, RVA 0x17450.
// Original XCurl.dll SHA256:
// 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551
//
// The original's 0x28020 dispatch points to this function. Reconstructing
// it makes curl_slist_append's string length/alloc/copy behavior explicit.
// Its two internal callees remain unresolved; these are DECLARATIONS, not
// substitute library calls or invented production implementations.
#include <cstddef>
#include <cstdint>

namespace t6_xcurl::internal {
void* allocate_tagged(std::size_t n, std::uint32_t tag);  // FUN_18000932C
void* copy_bytes(void* dst, const void* src, std::size_t n); // FUN_180004AD0

char* duplicate_string(const char* source) {
    // The original has no null guard, and scans to the first NUL byte.
    std::size_t length = 0;
    while (source[length] != '\0') ++length;
    const std::size_t bytes = length + 1;
    auto* result = static_cast<char*>(allocate_tagged(bytes, 5));
    if (result) copy_bytes(result, source, bytes);
    return result;
}
} // namespace t6_xcurl::internal
