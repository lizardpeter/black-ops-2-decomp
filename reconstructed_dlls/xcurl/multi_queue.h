#pragma once
#include "api_profile.h"
#include "option_dispatch.h"
namespace t6_xcurl {
using WaitOne=std::uint32_t(*)(void*,std::uint32_t,void*);
using WaitMany=std::uint32_t(*)(std::uint32_t,const void*,std::uint32_t,void*);
using EventAction=void(*)(void*,void*);
using EngineStep=void(*)(void*,void*);
using CancelEasy=void(*)(void*,std::uint32_t,std::uint32_t,void*);
using EasyDone=std::uint8_t(*)(void*,void*);
using GlobalAdjust=void(*)(std::uint32_t,void*);
struct MultiServices {
    Lock enter,leave;
    WaitOne wait_one;
    WaitMany wait_many;
    EventAction release_mutex,set_event;
    EngineStep advance_pending,advance_active;
    CancelEasy cancel_easy;
    EngineStep after_cancel,progress_easy;
    EasyDone easy_done;
    GlobalAdjust adjust_global;
    OptionFree release;
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_multi_add(void*,void*,const t6_xcurl::MultiServices*);
extern "C" std::uint32_t xcurl_research_multi_remove(void*,void*,const t6_xcurl::MultiServices*);
extern "C" std::uint32_t xcurl_research_multi_perform_inner(void*,std::uint32_t*,const t6_xcurl::MultiServices*);
extern "C" std::uint32_t xcurl_research_multi_poll_inner(void*,std::uint32_t,std::uint32_t*,const t6_xcurl::MultiServices*);
extern "C" void* xcurl_research_multi_info_read_inner(void*,std::uint32_t*,const t6_xcurl::MultiServices*);
