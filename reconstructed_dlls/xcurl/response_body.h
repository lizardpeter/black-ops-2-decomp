#pragma once
#include "request_engine.h"
namespace t6_xcurl {
// Symbolic callback identifiers, not addresses from the original DLL.
struct BodyServices {
    const RequestServices* request;
    std::uint32_t(*submit)(void*,void*,std::uint32_t,void*);
    std::uint32_t(*deliver)(void*,const void*,std::uint32_t,void*);
};
}
extern "C" {
std::uint32_t xcurl_research_reserve_body_buffer(void*,void**,const t6_xcurl::BodyServices*);
std::uint32_t xcurl_research_queue_read(void*,const t6_xcurl::BodyServices*);
std::uint32_t xcurl_research_schedule_read(void*,void*,const t6_xcurl::BodyServices*);
void xcurl_research_deferred_read(void*,const t6_xcurl::BodyServices*);
std::uint32_t xcurl_research_publish_body(void*,void*,std::uint32_t,const t6_xcurl::BodyServices*);
void xcurl_research_body_worker(void*,const t6_xcurl::BodyServices*);
}
namespace t6_xcurl {
struct DeliveryServices {
    const RequestServices* request;
    std::uint64_t(*write)(void*,const void*,std::uint64_t,void*,void*);
    void(*diagnose)(void*,std::uint32_t,std::uint64_t,std::uint32_t,void*);
};
}
extern "C" std::uint32_t xcurl_research_deliver_body(void*,const void*,std::uint32_t,const t6_xcurl::DeliveryServices*);
