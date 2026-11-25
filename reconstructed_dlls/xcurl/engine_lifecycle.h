#pragma once
#include "global_lifecycle.h"
#include "gdk_interfaces.h"
#include "session_cache.h"
#include "engine_scheduler.h"
namespace t6_xcurl {
// The original 0xA8-byte engine contains an intrusive multi list, two
// critical sections, a session cache and the runtime-owned notification handles.
// The host owns the callback entrypoints and must drain them before destruction.
inline constexpr std::size_t engine_bytes=0xa8;
struct EngineServices {
    const GdkQueryServices* gdk;
    const SessionServices* sessions;
    const SchedulerServices* scheduler;
    Lock initialize_cs,delete_cs,enter,leave;
    EventAction set_event,close_handle,release_mutex;
    WaitOne wait_one;
    CreateEventFn create_mutex;
    LastError last_error;
    std::uint32_t(*telemetry_create)(void**,void*);
    EventAction telemetry_destroy;
    std::uint32_t(*register_app_state)(void*,void*,void**,void*);
    EventAction unregister_app_state;
    void* connectivity_callback;
    void* app_state_callback;
    OptionAllocate allocate;
    OptionFree release;
    std::uint32_t(*initialize_runtime)(void*);
    std::uint32_t(*query_frequency)(std::uint64_t*,void*);
    void* context;
};
}
extern "C" {
void* xcurl_research_engine_construct(void*,const t6_xcurl::EngineServices*);
std::uint32_t xcurl_research_engine_initialize(void*,const t6_xcurl::EngineServices*);
void xcurl_research_engine_destroy(void*,const t6_xcurl::EngineServices*);
void xcurl_research_engine_connectivity(void*,const t6_xcurl::EngineServices*);
void xcurl_research_engine_set_limit(void*,std::uint32_t,const t6_xcurl::EngineServices*);
void xcurl_research_engine_suspend(void*,const t6_xcurl::EngineServices*);
void xcurl_research_engine_resume(void*,const t6_xcurl::EngineServices*);
void xcurl_research_engine_app_state(std::uint8_t,void*,const t6_xcurl::EngineServices*);
std::uint32_t xcurl_research_engine_create(t6_xcurl::GlobalLifecycle*,const t6_xcurl::EngineServices*);
std::uint32_t xcurl_research_engine_bootstrap(t6_xcurl::GlobalLifecycle*,std::uint32_t,std::uint64_t*,const t6_xcurl::EngineServices*);
std::uint32_t xcurl_research_verify_server_certificate(void*,const t6_xcurl::GdkQueryServices*);
}
