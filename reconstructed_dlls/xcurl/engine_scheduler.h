#pragma once
#include "request_engine.h"
namespace t6_xcurl {
struct SchedulerServices {
    Lock enter,leave;
    EngineStep start_easy,progress_easy;
    CancelEasy cancel_easy;
    EngineStep after_cancel;
    EventAction reset_event,set_event;
    WaitOne wait_one;
    void* context;
};
}
extern "C" {
void xcurl_research_advance_pending(void*,void*,const t6_xcurl::SchedulerServices*);
void xcurl_research_advance_active(void*,const t6_xcurl::SchedulerServices*);
void xcurl_research_abort_all(void*,const t6_xcurl::SchedulerServices*);
std::uint32_t xcurl_research_multi_wait(void*,std::uint32_t,std::uint32_t*,const t6_xcurl::SchedulerServices*);
std::uint32_t xcurl_research_multi_wakeup(void*,const t6_xcurl::SchedulerServices*);
}
