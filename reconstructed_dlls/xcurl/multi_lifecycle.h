#pragma once
#include "engine_scheduler.h"
namespace t6_xcurl {
// Original 0x184A8 and 0x18110 with ETW disabled. The scheduler and
// request services are already reconstructed; only OS creation/close
// and mutex release remain host boundaries.
struct MultiLifecycleServices {
    const SchedulerServices* scheduler;
    const RequestServices* request;
    CreateEventFn create_mutex;
    void* (*create_event)(std::uint8_t manual_reset,std::uint8_t initial_state,void*);
    EventAction close_handle,release_mutex;
    LastError last_error;
    void* context;
};
}
extern "C" {
void xcurl_research_easy_stop(void*,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_multi_start_engine(void*,void*,const t6_xcurl::MultiLifecycleServices*);
std::uint32_t xcurl_research_multi_destroy_engine(void*,void*,const t6_xcurl::MultiLifecycleServices*);
}
