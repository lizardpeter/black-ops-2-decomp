// VC100 (VS2010) <stdio.h>, as far as the reconstruction needs it.
//
// Build environment, not game source. The functions resolve to the original binary's CRT through the hybrid link.
// On x64 the UCRT defines _vsnprintf inline (over __stdio_common_vsprintf); the Store build's copy is identified
// like any other function.
#pragma once

#include <stdarg.h>

typedef __SIZE_TYPE__ size_t;

#ifdef __cplusplus
extern "C" {
#endif

int _vsnprintf(char *buffer, size_t count, const char *format, va_list argptr);

#ifdef __cplusplus
}
#endif
