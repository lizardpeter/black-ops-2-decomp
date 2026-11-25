#pragma once
#include "multi_queue.h"
namespace t6_xcurl {
using CreateEventFn=void*(*)(void*);
using LastError=std::uint32_t(*)(void*);
using CreateQueue=std::uint32_t(*)(void**,void*);
using Diagnostic=void(*)(void*,std::uint32_t,std::uint32_t,void*);
using QueueDispatch=std::uint8_t(*)(void*,std::uint32_t,std::uint32_t,void*);
using HttpRead=std::uint32_t(*)(void*,void*,std::uint32_t,void*);
using HttpReceive=std::uint32_t(*)(void*,void*);
using HttpSend=std::uint32_t(*)(void*,const void*,std::uint32_t,std::uint32_t,void*,void*);
using HttpSetContext=std::uint32_t(*)(void*,void*,void*);
using HttpCallback=std::uintptr_t(*)(void*,std::uint32_t,void*);
using DebugData=void(*)(void*,std::uint32_t,const void*,std::uint64_t,void*);
struct RequestServices {
    Lock enter,leave;
    OptionAllocate allocate;
    OptionFree release;
    EventAction set_event,close_http,release_connection;
    GlobalAdjust adjust_global;
    CreateEventFn create_event;
    LastError last_error;
    CreateQueue create_queue;
    Diagnostic diagnostic;
    QueueDispatch queue_dispatch;
    HttpRead read;
    HttpReceive receive;
    HttpSend send;
    HttpSetContext set_context;
    HttpCallback set_callback;
    DebugData debug_data;
    EngineStep cancel_task;
    std::uint32_t(*submit_completion)(void*,void*,void*);
    void* context;
};
}
extern "C" {
std::uint32_t xcurl_research_easy_start_engine(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_progress(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_cancel(void*,std::uint32_t,std::uint32_t,const t6_xcurl::RequestServices*);
std::uint8_t xcurl_research_easy_done(void*,const t6_xcurl::RequestServices*);
void xcurl_research_request_reset(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_complete(void*,const t6_xcurl::RequestServices*);
void xcurl_bound_easy_complete(void*,void*,const t6_xcurl::RequestServices*);
void xcurl_research_request_close(void*,const t6_xcurl::RequestServices*);
void xcurl_research_request_stop(void*,const t6_xcurl::RequestServices*);
void xcurl_research_request_finish(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_notify(void*,const t6_xcurl::RequestServices*);
void xcurl_research_easy_schedule_completion(void*,const t6_xcurl::RequestServices*);
void xcurl_research_engine_decrement(void*,std::uint32_t,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_http_read(void*,void*,std::uint32_t,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_http_receive(void*,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_http_send(void*,const void*,std::uint64_t,std::uint64_t,const t6_xcurl::RequestServices*);
std::uint32_t xcurl_research_http_register(void*,const t6_xcurl::RequestServices*);
}
