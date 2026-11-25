#pragma once
#include "engine_lifecycle.h"
#include "easy_setopt.h"
namespace t6_xcurl {
// Source-link integration, not the exported retail ABI. The caller owns
// the lifetime of all services and serializes global init/cleanup (including
// implicit init by future easy/multi wrappers). No live handles at cleanup.
struct ApiBinding {
    GlobalLifecycle global{};
    ApiProfile profile{};
    std::uint64_t frequency=0;
    std::uint32_t engine_limit=0;
    const EngineServices* engine=nullptr;
    void* profile_lock=nullptr;
    Lock enter=nullptr,leave=nullptr;
    Tick tick=nullptr;
    ErrorLog error=nullptr;
    ActiveRequestLog active_log=nullptr;
    GlobalShutdown shutdown_runtime=nullptr;
    void* context=nullptr;
    BootstrapReport startup{};
};
std::uint32_t bound_global_init(ApiBinding&);
void bound_global_cleanup(ApiBinding&);
std::uint32_t bound_easy_setopt(ApiBinding&,void*,int,std::uint64_t);
std::uint32_t bound_easy_getinfo(ApiBinding&,const void*,int,void*);
}
