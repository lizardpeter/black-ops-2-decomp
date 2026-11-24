#pragma once
#include "api_profile.h"
namespace t6_xcurl {
// Exact disabled-ETW control flow for 0xE460 and 0xCBF8. The actual
// option dispatcher 0xEAF8 and active-request diagnostic 0xA224 are
// explicit boundaries. args points to an original Windows x64-style
// 8-byte argument slot; no host varargs ABI is assumed.
using OptionDispatch=std::uint32_t(*)(void* options,int option,const std::uint64_t* args,void* context);
using ActiveRequestLog=void(*)(void* easy,void* context);
}
extern "C" std::uint32_t xcurl_research_easy_setopt(void* easy,int option,std::uint64_t argument,
    t6_xcurl::ApiProfile*,void* profile_lock,t6_xcurl::Lock,t6_xcurl::Lock,t6_xcurl::Tick,
    t6_xcurl::OptionDispatch,t6_xcurl::ActiveRequestLog,t6_xcurl::ErrorLog,void*);
extern "C" std::uint32_t xcurl_research_setopt_locked(void* easy,int option,const std::uint64_t* args,
    t6_xcurl::Lock,t6_xcurl::Lock,t6_xcurl::OptionDispatch,t6_xcurl::ActiveRequestLog,void*);
