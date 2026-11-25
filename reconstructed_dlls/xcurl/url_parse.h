#pragma once
#include "url_component.h"
namespace t6_xcurl {
// Original 1C780 flag-zero HTTP/HTTPS path. The file:// branch, relative
// resolution into an already populated CURLU, and nonzero encoding flags
// are not part of this entrypoint.
}
extern "C" {
std::uint32_t xcurl_research_url_control_check(const char*);
char* xcurl_research_url_normalize_path(const char*,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_parse_port(void*,char*,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_parse_http(const char*,void*,const t6_xcurl::UrlObjectServices*);
std::uint32_t xcurl_research_url_set_http(void*,int,const char*,const t6_xcurl::UrlObjectServices*);
}
