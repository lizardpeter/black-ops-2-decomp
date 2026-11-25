#pragma once
#include "url_object.h"
namespace t6_xcurl {
inline constexpr std::uint32_t url_default_port=1;
inline constexpr std::uint32_t url_no_default_port=2;
inline constexpr std::uint32_t url_default_scheme=4;
inline constexpr std::uint32_t url_non_supported_scheme=8;
// The original binary's scheme descriptors at 0x1F1E0/0x1F270.
std::uint32_t url_scheme_port(const char*);
}
extern "C" {
std::uint32_t xcurl_research_url_validate_host(void*,char*,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_set_part(void*,int,const char*,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_format(void*,char**,unsigned,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_get_flags(void*,int,char**,unsigned,const t6_xcurl::UrlObjectServices*);
}
