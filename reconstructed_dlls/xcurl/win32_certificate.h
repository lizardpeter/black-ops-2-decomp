#pragma once
#ifdef _WIN32
#include "winhttp_services.h"
namespace t6_xcurl {
// Native integration for the original certificate selector. Fail closed if
// an explicitly requested client certificate cannot be selected or installed.
std::uint32_t win32_select_client_certificate(void*,const char16_t*,WinHttpHost&);
}
#endif
