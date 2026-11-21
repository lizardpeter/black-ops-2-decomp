// VC100 (VS2010) <stdlib.h>, as far as the reconstruction needs it.
//
// Build environment, not game source. The functions resolve to the original binary's CRT through the hybrid link.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int atoi(const char *str);
double atof(const char *str);
__int64 _atoi64(const char *str);

#ifdef __cplusplus
}
#endif
