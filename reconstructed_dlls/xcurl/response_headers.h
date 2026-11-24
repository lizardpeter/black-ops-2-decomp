#pragma once
#include "header_query.h"
namespace t6_xcurl {
struct ReceiveHeaderServices {
    const HeaderQueryServices* query;
    const RequestServices* request;
    // Original 0x14AC0 (URL_COMPONENTS decode and insecure-scheme diagnostic).
    std::uint32_t(*crack_scheme)(const char16_t*,void*,std::uint32_t*,void*);
    void(*diagnose)(void*,std::uint32_t,std::uint64_t,std::uint32_t,void*);
};
}
extern "C" std::uint32_t xcurl_research_receive_headers(void*,void*,std::uint32_t*,const t6_xcurl::ReceiveHeaderServices*);
