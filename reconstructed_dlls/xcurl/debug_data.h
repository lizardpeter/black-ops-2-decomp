#pragma once
#include <cstdint>
namespace t6_xcurl {
struct DebugServices {
    void(*output)(const char*,void*);
    void(*write)(const void*,std::uint64_t,std::uint64_t,void*,void*);
    void* context;
};
}
// Pinned 0xA128. Custom callback takes precedence over default output.
extern "C" int xcurl_research_debug_data(void*,int,const void*,std::uint64_t,const t6_xcurl::DebugServices*);
