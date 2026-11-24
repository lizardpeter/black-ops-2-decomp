#pragma once
#include <cstdint>
namespace t6_xcurl {
// Recovered profiling fields 0x28BA0 and 0x28BB0+4*API ID.
// 24 DWORD slots end exactly at the engine pointer (0x28C10).
// Only IDs recovered from the original caller are used here.
struct ApiProfile {
    std::uint64_t first_tick=0;
    std::uint32_t counts[24]{};
};
using Lock=void(*)(void* address,void* context);
using Tick=std::uint64_t(*)(void* context);
using ErrorLog=void(*)(std::uint32_t code,std::uint32_t api,void* context);
}
extern "C" std::uint32_t xcurl_easy_getinfo_unlocked(const void*,int,void**);
extern "C" void xcurl_research_record_api(t6_xcurl::ApiProfile*,std::uint32_t id,
    void* profile_lock,t6_xcurl::Lock enter,t6_xcurl::Lock leave,t6_xcurl::Tick tick,void* ctx);
extern "C" std::uint32_t xcurl_research_easy_getinfo(const void* easy,int option,void* out,
    t6_xcurl::ApiProfile*,void* profile_lock,t6_xcurl::Lock enter,t6_xcurl::Lock leave,
    t6_xcurl::Tick tick,t6_xcurl::ErrorLog error_log,void* ctx);
