#pragma once
#include "request_engine.h"
#include "gdk_interfaces.h"
namespace t6_xcurl {
// Original URL conversion and asynchronous start boundaries. The callback token
// must be the actual native translation of 0x11F10 when integrated.
struct PrepareServices {
    const RequestServices* request;
    std::uint32_t(*prepare_url)(void*,void*,void*,void*);
    std::uint32_t(*default_url)(const void*,std::uint32_t,void**,void*);
    std::uint32_t(*parse_port)(const void*,void*);
    void(*url_diagnostic)(void*,std::uint32_t,std::uintptr_t,std::uint32_t,void*);
    std::uint32_t(*copy_url)(const void*,void*,void*);
    const GdkQueryServices* gdk;
    void* engine_queue;
    void* start_callback;
};
}
extern "C" {
void xcurl_research_easy_reset_transfer(void*,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_easy_prepare_url(void*,const t6_xcurl::PrepareServices*);
void xcurl_research_request_begin(void*,const t6_xcurl::PrepareServices*);
std::uint32_t xcurl_research_easy_begin(void*,const t6_xcurl::PrepareServices*);
std::uint32_t xcurl_bound_easy_begin(void*,const t6_xcurl::PrepareServices*);
}
