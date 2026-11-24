// Pinned Store binkw64.dll 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17.
// Export trampolines 0x211D0/E0/F0 -> 0x21DE0/0x220A0/0x22140.
// Platform queue/thread creation, destruction and joining remain explicit boundaries.
#include "async_lifecycle.h"
using namespace t6_bink;
extern "C" int bink_research_start_worker(AsyncLifecycle* s,std::uint32_t n,
    WorkerQueueInit init,WorkerQueueDestroy destroy,WorkerLaunch launch,void* ctx) {
    if(!s||!init||!destroy||!launch)return 0;
    s->error_rva=0;
    if(n>=8){s->error_rva=0x5F530;return 0;}
    const auto bit=1U<<n;
    if(s->running&bit){s->error_rva=0x5F550;return 0;}
    if(!init(n,0,ctx))return 0;
    if(!init(n,1,ctx)||!launch(n,ctx)) {
        // Original failure path destroys the first queue only.
        destroy(n,0,ctx);return 0;
    }
    s->running|=bit;return 1;
}
extern "C" int bink_research_stop_worker(AsyncLifecycle* s,std::uint32_t n,WorkerStop stop,void* ctx) {
    if(!s||!stop)return 0;
    s->error_rva=0;
    if(n>=8){s->error_rva=0x5F530;return 0;}
    const auto bit=1U<<n;
    if(!(s->running&bit)){s->error_rva=0x5F578;return 0;}
    if(!stop(n,0,ctx))return 0;
    s->running&=~bit;s->stop_requested|=bit;return 1;
}
extern "C" int bink_research_join_worker(AsyncLifecycle* s,std::uint32_t n,
    WorkerJoin join,WorkerQueueDestroy destroy,void* ctx) {
    if(!s||!join||!destroy)return 0;
    s->error_rva=0;
    if(n>=8){s->error_rva=0x5F530;return 0;}
    if(!(s->stop_requested&(1U<<n))){s->error_rva=0x5F590;return 0;}
    if(!join(n,ctx))return 0;
    destroy(n,0,ctx);destroy(n,1,ctx);
    // Original export does not clear stop_requested on success.
    return 1;
}
