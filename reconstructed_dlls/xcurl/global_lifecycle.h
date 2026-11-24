#pragma once
#include "api_profile.h"
#include <cstdint>
namespace t6_xcurl {
// Original global refcount at 0x28C18 and request-engine pointer at 0x28C10.
struct GlobalLifecycle {
    void* engine=nullptr;
    std::uint32_t references=0;
};
using EngineInit=std::uint32_t(*)(GlobalLifecycle*,void*);
using EngineDestroy=void(*)(void*,void*);
using TaggedFree=void(*)(void*,std::uint32_t,void*);
using GlobalShutdown=void(*)(void*);
}
extern "C" std::uint32_t xcurl_research_global_init(t6_xcurl::GlobalLifecycle*,
    t6_xcurl::ApiProfile*,void*,t6_xcurl::Lock,t6_xcurl::Lock,t6_xcurl::Tick,
    t6_xcurl::EngineInit,t6_xcurl::ErrorLog,void*);
extern "C" void xcurl_research_global_cleanup(t6_xcurl::GlobalLifecycle*,
    t6_xcurl::ApiProfile*,void*,t6_xcurl::Lock,t6_xcurl::Lock,t6_xcurl::Tick,
    t6_xcurl::EngineDestroy,t6_xcurl::TaggedFree,t6_xcurl::GlobalShutdown,void*);
