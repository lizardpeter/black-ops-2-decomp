#pragma once
#include "request_engine.h"
namespace t6_xcurl {
struct AddHeaderServices {
    const RequestServices* request;
    std::uint32_t (*add)(void*,const char16_t*,std::uint32_t,std::uint32_t,void*);
};
}
extern "C" std::uint32_t xcurl_research_add_request_headers(void*,const void*,const t6_xcurl::AddHeaderServices*);
