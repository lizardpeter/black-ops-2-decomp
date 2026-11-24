#pragma once
#include "api_profile.h"
#include "option_dispatch.h"
namespace t6_xcurl {
// Explicit original engine boundaries: 0x9BA0, 0xB55C, 0xA544,
// 0x184A8 and 0x18110. No request engine substituted here.
using GlobalStart=std::uint32_t(*)(void*);
using HandleStart=std::uint32_t(*)(void*,void*);
using HandleDestroy=void(*)(void*,void*);
using MultiDestroy=std::uint32_t(*)(void*,void*);
struct HandleServices {
    OptionAllocate allocate;
    OptionFree release;
    Lock initialize_lock;
    GlobalStart start_global;
    HandleStart start_easy;
    HandleDestroy destroy_easy;
    HandleStart start_multi;
    MultiDestroy destroy_multi;
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_easy_create(void**,std::uint32_t*,const t6_xcurl::HandleServices*);
extern "C" void xcurl_research_easy_destroy(void*,const t6_xcurl::HandleServices*);
extern "C" std::uint32_t xcurl_research_multi_create(void**,const t6_xcurl::HandleServices*);
extern "C" std::uint32_t xcurl_research_multi_destroy(void*,const t6_xcurl::HandleServices*);
extern "C" void xcurl_research_options_destroy(void*,t6_xcurl::OptionFree,void*);
