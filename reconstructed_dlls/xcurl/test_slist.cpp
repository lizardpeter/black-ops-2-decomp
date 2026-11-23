// Isolated control-flow tests. The allocator/test doubles are NOT reconstructions.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "slist.cpp"

namespace {
int allocations = 0, frees = 0;
bool fail_duplicate = false, fail_node = false;
}
namespace t6_xcurl::internal {
char* duplicate_string(const char* data) {
    if (fail_duplicate) return nullptr;
    auto n = std::strlen(data) + 1;
    auto* out = static_cast<char*>(std::malloc(n));
    if (out) std::memcpy(out, data, n);
    return out;
}
void* allocate_tagged(std::size_t n, std::uint32_t tag) {
    assert(n == 16 && tag == 5);
    ++allocations;
    return fail_node ? nullptr : std::malloc(n);
}
void free_tagged(void* ptr, std::uint32_t tag) {
    assert(ptr && tag == 5);
    ++frees;
    std::free(ptr);
}
}
int main() {
    auto* head = curl_slist_append(nullptr, "alpha");
    assert(head && std::strcmp(head->data, "alpha") == 0 && !head->next);
    auto* again = curl_slist_append(head, "beta");
    assert(again == head && head->next && std::strcmp(head->next->data, "beta") == 0);
    fail_duplicate = true;
    assert(curl_slist_append(head, "fail") == nullptr);
    fail_duplicate = false;
    fail_node = true;
    auto before = frees;
    assert(curl_slist_append(head, "fail") == nullptr);
    assert(frees == before + 1); // Original frees duplicated string on node allocation failure
    fail_node = false;
    curl_slist_free_all(head);
    assert(frees == 5); // 2 strings + 2 nodes + failed string
    curl_slist_free_all(nullptr);
}
