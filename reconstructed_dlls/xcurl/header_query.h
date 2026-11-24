#pragma once
#include "request_engine.h"
namespace t6_xcurl {
struct HeaderQueryServices {
    OptionAllocate allocate;
    OptionFree release;
    std::uint32_t(*query)(void*,std::uint32_t,void*,std::uint32_t*,void*);
    LastError error;
    // Native boundary: WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, ..., nullptr, nullptr).
    int(*utf8)(const char16_t*,int,char*,int,void*);
    void* context;
};
}
extern "C" {
std::uint32_t xcurl_research_query_wide(void*,std::uint32_t,void**,std::uint64_t*,const t6_xcurl::HeaderQueryServices*);
std::uint32_t xcurl_research_header_utf8(const char16_t*,void**,std::uint64_t*,const t6_xcurl::HeaderQueryServices*);
std::uint32_t xcurl_research_query_utf8(void*,std::uint32_t,void**,std::uint64_t*,const t6_xcurl::HeaderQueryServices*);
void xcurl_research_publish_header_fields(void*,void**,std::uint32_t,std::uint8_t,std::uint64_t,std::uint64_t,void**,std::uint32_t,const char*,std::uint32_t,void**,std::uint32_t,const t6_xcurl::RequestServices*);
}
