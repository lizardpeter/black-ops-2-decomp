#pragma once
#include <cstddef>
#include <cstdint>
namespace t6_xcurl {
// Request options occupy easy+0x78. Only the disabled-ETW branch is ported.
// Explicit boundaries correspond to original tagged heap and UTF-8 -> UTF-16
// allocation routine 0x1AC88. No HTTP transport or easy lifetime is implied.
using OptionAllocate=void*(*)(std::size_t,std::uint32_t,void*);
using OptionFree=void(*)(void*,std::uint32_t,void*);
using OptionWideString=std::uint32_t(*)(void* pointer_slot,const char*,std::uint32_t,void*);
struct OptionServices {
    OptionAllocate allocate;
    OptionFree release;
    OptionWideString wide_string;
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_dispatch_option(void*,int,const std::uint64_t*,const t6_xcurl::OptionServices*);
