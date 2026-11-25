#pragma once
#include "request_prepare.h"
namespace t6_xcurl {
struct EasyLifecycleServices {
    const RequestServices* request;
    const GdkQueryServices* gdk;
    Lock delete_cs;
    EventAction close_handle;
    void* context;
};
}
extern "C" {
std::uint8_t xcurl_research_request_is_active(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_destroy_engine(void*,const t6_xcurl::EasyLifecycleServices*);
}
