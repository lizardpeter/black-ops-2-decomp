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

// setjmp is MSVC's x64 _setjmp(env, frame): the frame is the establisher frame longjmp unwinds to (RDX). Clang lowers
// a call to _setjmp that way itself, but not with -fno-builtin (tools/build_recon.py), so the frame is passed
// explicitly: __builtin_frame_address(0) is the same establisher frame on Win64 (llvm.frameaddress), as clang's own
// lowering uses.
#ifdef _M_X64
extern "C" __attribute__((returns_twice)) int __cdecl _setjmp(jmp_buf env, void *frame);
#define setjmp(env) _setjmp((env), __builtin_frame_address(0))
#endif
