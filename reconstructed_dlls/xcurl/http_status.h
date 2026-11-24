#pragma once
#include "request_engine.h"
namespace t6_xcurl {
struct StatusServices {
    const RequestServices* request;
    std::uint32_t(*sending)(void*,void*);
    std::uint32_t(*redirect)(const void*,void*,void*);
    std::uint32_t(*continue_upload)(void*,void*);
    std::uint32_t(*headers)(void*,void*,std::uint32_t*,void*);
    std::uint32_t(*publish_headers)(void*,void*);
    std::uint32_t(*publish_data)(void*,const void*,std::uint32_t,void*);
    std::uint32_t(*schedule_read)(void*,void*,void*);
    std::uint32_t(*query_available)(void*,void*);
    void(*diagnostic)(void*,std::uint32_t,std::uint64_t,std::uint64_t,std::uint32_t,void*);
};
}
extern "C" void xcurl_research_http_status(void*,std::uint32_t,const void*,std::uint32_t,const t6_xcurl::StatusServices*);
