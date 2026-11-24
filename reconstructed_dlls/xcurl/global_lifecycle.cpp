// Original Store XCurl x64, SHA256 65cd27b336abecf821896611c91e8ec74d0328bd65ec11aa5ef4e2affbdd4551.
// Public global init 0x9E30, cleanup 0x9CD0, accounting 0x88A0.
// Original request-engine init (0x9BA0), engine shutdown (0x165D0)
// and process cleanup (0x5568) are explicit unresolved boundaries.
// This is the disabled-ETW provider path, not a usable networking runtime.
#include "global_lifecycle.h"
#include <atomic>
using namespace t6_xcurl;
extern "C" std::uint32_t xcurl_research_global_init(GlobalLifecycle* s,
    ApiProfile* p,void* lock,Lock enter,Lock leave,Tick tick,
    EngineInit init,ErrorLog log,void* ctx) {
    xcurl_research_record_api(p,0,lock,enter,leave,tick,ctx);
    std::atomic_ref<std::uint32_t> refs(s->references);
    if(refs.fetch_add(1)!=0)return 0;
    // Original leaves reference count incremented even on init failure.
    const auto status=init(s,ctx);
    if(status&&log)log(status,0,ctx);
    return status;
}
extern "C" void xcurl_research_global_cleanup(GlobalLifecycle* s,
    ApiProfile* p,void* lock,Lock enter,Lock leave,Tick tick,
    EngineDestroy destroy,TaggedFree free_fn,GlobalShutdown shutdown,void* ctx) {
    xcurl_research_record_api(p,3,lock,enter,leave,tick,ctx);
    std::atomic_ref<std::uint32_t> refs(s->references);
    if(refs.load()==0)return;
    if(refs.fetch_sub(1)!=1)return;
    destroy(s->engine,ctx);
    free_fn(s->engine,11,ctx);
    s->engine=nullptr;
    shutdown(ctx);
}
