#pragma once
#include "url_crack.h"
namespace t6_xcurl {
// Original 0x10EA4 coordinator. CURLU allocation/set/get/destroy remain
// explicit boundaries until the embedded URL parser itself is recovered.
struct UrlPreparationServices {
    void*(*create)(void*);
    std::uint32_t(*set)(void*,int,const char*,unsigned,void*);
    std::uint32_t(*get)(void*,int,char**,unsigned,void*);
    void(*destroy)(void*,void*);
    OptionAllocate allocate;
    OptionFree release;
    void(*diagnose)(void*,std::uint32_t,std::uintptr_t,std::uint32_t,void*);
    const UrlServices* crack;
    void* context;
};
}
extern "C" std::uint32_t xcurl_research_prepare_url(const char*,void*,void*,const t6_xcurl::UrlPreparationServices*);
