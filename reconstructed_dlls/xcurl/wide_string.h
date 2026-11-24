#pragma once
#include "option_dispatch.h"
namespace t6_xcurl {
// Original 0x1AC88: UTF-8 -> UTF-16 string ownership. `capacity` and
// `written` are optional counts of UTF-16 code units (excluding NUL).
}
extern "C" std::uint32_t xcurl_research_wide_string(void* slot,const char* text,std::uint32_t tag,
    std::uint64_t* capacity,std::uint64_t* written,t6_xcurl::OptionAllocate,t6_xcurl::OptionFree,void* context);
extern "C" std::uint32_t xcurl_research_dispatch_owned_option(void*,int,const std::uint64_t*,
    t6_xcurl::OptionAllocate,t6_xcurl::OptionFree,void* context);
