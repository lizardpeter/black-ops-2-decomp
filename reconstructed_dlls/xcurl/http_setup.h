#pragma once
#include "request_engine.h"
namespace t6_xcurl {
// Disabled-ETW WinHTTP setup. OS operations are injected at their actual boundaries.
struct HttpSetupServices {
    void* (*connect)(void*,const char16_t*,std::uint16_t,void*);
    void* (*open)(void*,const char16_t*,const char16_t*,const char16_t* const*,std::uint32_t,void*);
    std::uint32_t (*option)(void*,std::uint32_t,std::uint32_t,void*);
    std::uint32_t (*timeouts)(void*,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,void*);
    LastError error;
    Diagnostic diagnostic;
    void (*retry_diagnostic)(void*,void*);
    void* context;
};
}
extern "C" {
void xcurl_research_upload_mode(void*,std::uint32_t);
std::uint32_t xcurl_research_http_open(void*,void*,const char16_t*,void*,const char16_t* const*,std::uint16_t,const t6_xcurl::HttpSetupServices*);
std::uint32_t xcurl_research_http_options(void*,const void*,std::uint32_t,const t6_xcurl::HttpSetupServices*);
}
namespace t6_xcurl {
using AcquireSession=std::uint32_t(*)(void*,void*,std::uint32_t,std::uint32_t,void**,void*);
}
extern "C" std::uint32_t xcurl_research_http_open_with_session(void*,void*,void*,void*,t6_xcurl::AcquireSession,void*,const t6_xcurl::HttpSetupServices*);
extern "C" std::uint32_t xcurl_research_copy_utf16(const char16_t*,void**,t6_xcurl::OptionAllocate,void*);
