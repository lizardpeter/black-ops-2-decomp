// VC100 (VS2010) <setjmp.h>: the jmp_buf type, as far as the reconstruction needs it.
//
// Build environment, not game source.
#pragma once

#ifdef _M_X64
typedef struct __declspec(align(16)) _SETJMP_FLOAT128 { unsigned __int64 Part[2]; } SETJMP_FLOAT128;
typedef SETJMP_FLOAT128 jmp_buf[16];
#else
typedef int jmp_buf[16];
#endif
