#pragma once
#include <cstdint>
namespace t6_bink {
// Original global state at 0x570A4, 0x570BC and 0x57028.
// error_rva describes the exact original static error string, not a host pointer.
struct AsyncLifecycle {
    std::uint32_t running=0,stop_requested=0,error_rva=0;
};
using WorkerQueueInit=int(*)(std::uint32_t,unsigned,void*);
using WorkerQueueDestroy=void(*)(std::uint32_t,unsigned,void*);
using WorkerLaunch=int(*)(std::uint32_t,void*);
using WorkerStop=int(*)(std::uint32_t,std::uintptr_t,void*);
using WorkerJoin=int(*)(std::uint32_t,void*);
}
extern "C" int bink_research_start_worker(t6_bink::AsyncLifecycle*,std::uint32_t,
    t6_bink::WorkerQueueInit,t6_bink::WorkerQueueDestroy,t6_bink::WorkerLaunch,void*);
extern "C" int bink_research_stop_worker(t6_bink::AsyncLifecycle*,std::uint32_t,t6_bink::WorkerStop,void*);
extern "C" int bink_research_join_worker(t6_bink::AsyncLifecycle*,std::uint32_t,
    t6_bink::WorkerJoin,t6_bink::WorkerQueueDestroy,void*);
