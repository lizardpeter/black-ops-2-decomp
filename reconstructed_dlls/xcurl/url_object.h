#pragma once
#include "option_dispatch.h"
namespace t6_xcurl {
// Pinned CURLU layout, not the public libcurl implementation. Twelve owned
// string slots followed by the numeric port and four bytes of tail padding.
struct UrlObject {
    char* fields[12];
    std::uint32_t port;
    std::uint32_t padding;
};
static_assert(sizeof(UrlObject)==0x68);
struct UrlObjectServices {
    OptionAllocate allocate;
    OptionFree release;
    char*(*duplicate)(const char*,void*);
    void* context;
};
}
extern "C" {
void* xcurl_research_url_create(const t6_xcurl::UrlObjectServices*);
void xcurl_research_url_release_fields(void*,const t6_xcurl::UrlObjectServices*);
void xcurl_research_url_destroy(void*,const t6_xcurl::UrlObjectServices*);
void* xcurl_research_url_duplicate(const void*,const t6_xcurl::UrlObjectServices*);
void xcurl_research_url_move(void*,void*,const t6_xcurl::UrlObjectServices*);
std::uint8_t xcurl_research_url_has_scheme(const char*,char*);
// Original subset: component null setter and flag-zero getter for all
// individual parts, including lazy "/" allocation. Full URL format, decode,
// and default-port flags are not handled by these narrow entrypoints.
std::uint32_t xcurl_research_url_clear_part(void*,int,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_get_part(void*,int,char**,const t6_xcurl::UrlObjectServices*);
}
