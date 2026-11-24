#pragma once
#include "request_engine.h"
namespace t6_xcurl {
struct UrlServices {
    std::uint32_t(*crack)(const char16_t*,void*,void*);
    LastError error;
    void(*diagnose)(void*,std::uint32_t,const char16_t*,std::uint32_t,void*);
    char*(*format_warning)(const char16_t*,void*);
    void(*report)(std::uint32_t,const char*,void*);
    OptionFree release;
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_crack_url(const char16_t*,void*,void*,const t6_xcurl::UrlServices*);
