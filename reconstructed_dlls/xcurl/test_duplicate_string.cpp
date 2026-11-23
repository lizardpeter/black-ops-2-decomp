// Unit-only XCurl string duplicator tests with explicit test doubles for
// unresolved original allocator/copy callees. Not a DLL equivalence claim.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include "duplicate_string.cpp"

namespace {
std::size_t last_allocation = 0;
std::size_t last_copy = 0;
std::uint32_t last_tag = 0;
bool fail_allocation = false;
}
namespace t6_xcurl::internal {
void* allocate_tagged(std::size_t n, std::uint32_t tag) {
    last_allocation = n;
    last_tag = tag;
    return fail_allocation ? nullptr : std::malloc(n);
}
void* copy_bytes(void* dst, const void* src, std::size_t n) {
    last_copy = n;
    return std::memcpy(dst, src, n);
}
}
int main() {
    for (const char* value : {"", "a", "abc", "a longer string with spaces"}) {
        last_allocation = 0;
        last_copy = 0;
        char* p = t6_xcurl::internal::duplicate_string(value);
        const auto want = std::strlen(value) + 1;
        assert(p && last_allocation == want && last_copy == want);
        assert(last_tag == 5 && std::strcmp(p, value) == 0);
        std::free(p);
    }
    fail_allocation = true;
    last_copy = 0;
    assert(t6_xcurl::internal::duplicate_string("failure") == nullptr);
    assert(last_copy == 0);
}
