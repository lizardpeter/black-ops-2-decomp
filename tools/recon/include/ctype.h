// VC100 (VS2010) <ctype.h>, as far as the reconstruction needs it.
//
// Build environment, not game source. tolower is a real function in the original (_tolower), called with a char
// sign-extended to int, as here.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int tolower(int c);

#ifdef __cplusplus
}
#endif
