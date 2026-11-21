// <math.h>, as far as the reconstruction needs it.
//
// Build environment, not game source: it reproduces what the original compiler saw, so the reconstructed code can
// be written the way the original was.
//
// x86 (the VS2010 servers, /arch:SSE2): the float functions VC100 has SSE2 routines for are real functions here,
// forwarded to the original's ___libm_sse2_* copies by tools/recon/crt_sse2.s (the server calls
// ___libm_sse2_sinf directly, 90 call sites). atan2f, which has no float routine, is VC100's inline wrapper around
// the double function; floorf and ceilf are assumed to be too (check the disassembly at their first use). sqrtf and
// fabsf are intrinsics (below).
//
// x64 (the VS2019 Store build): the UCRT's functions, imported by the exe and reached through its import table.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double cos(double);
double sin(double);
double tan(double);
double exp(double);
double log(double);
double pow(double, double);
double sqrt(double);
double floor(double);
double ceil(double);
double fabs(double);
double fmod(double, double);

float acosf(float);
float asinf(float);
float atanf(float);
float cosf(float);
float expf(float);
float logf(float);
float log10f(float);
float powf(float, float);
float sinf(float);
float tanf(float);

#ifdef _M_X64
float atan2f(float, float);
float floorf(float);
float ceilf(float);
float fmodf(float, float);
#endif

#ifdef __cplusplus
}
#endif

#ifndef _M_X64
static inline float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }
static inline float floorf(float x) { return (float)floor((double)x); }
static inline float ceilf(float x) { return (float)ceil((double)x); }
#endif
// Intrinsics, as VC100 treats them: sqrtss / andps in SSE code, fsqrt / fabs where the expression is on the x87
// stack (X87_MATH, t6_compiler.h). A function would round its argument and result to float in between.
#define sqrtf(x) __builtin_sqrtf(x)
#define fabsf(x) __builtin_fabsf(x)
