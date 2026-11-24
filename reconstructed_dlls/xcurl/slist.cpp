// Direct reconstruction of exported XCurl list-manipulation *caller* bodies.
// Original XCurl.dll SHA256:
// 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551
// These bodies link the separately recovered allocation/duplication operations
// (RVA 0x932C, 0x9640, and indirect slot 0x28020) in the disabled-ETW path.
// NOT a complete XCurl implementation.
#include <cstddef>
#include <cstdint>

#include "xcurl_leaf_api.h"

namespace t6_xcurl::internal {
// Separately reconstructed original internal functions.
char* duplicate_string(const char* data);  // indirect allocator slot RVA 0x28020
void* allocate_tagged(std::size_t size, std::uint32_t tag); // RVA 0x932C
void free_tagged(void* pointer, std::uint32_t tag);        // RVA 0x9640
}

// RVA 0x1BB80: copy string, allocate 16-byte node with tag 5, append to tail.
// Preserve original all-or-nothing return on either allocation failure.
extern "C" curl_slist* curl_slist_append(curl_slist* list, const char* data) {
    char* duplicate = t6_xcurl::internal::duplicate_string(data);
    if (!duplicate) return nullptr;
    auto* node = static_cast<curl_slist*>(t6_xcurl::internal::allocate_tagged(16, 5));
    if (!node) {
        t6_xcurl::internal::free_tagged(duplicate, 5);
        return nullptr;
    }
    node->data = duplicate;
    node->next = nullptr;
    if (!list) return node;
    curl_slist* last = list;
    while (last->next) last = last->next;
    last->next = node;
    return list;
}

// RVA 0x1BC00: free each string (when non-null), clear data pointer, then node.
extern "C" void curl_slist_free_all(curl_slist* list) {
    while (list) {
        char* data = list->data;
        curl_slist* next = list->next;
        if (data) t6_xcurl::internal::free_tagged(data, 5);
        list->data = nullptr;
        t6_xcurl::internal::free_tagged(list, 5);
        list = next;
    }
}
