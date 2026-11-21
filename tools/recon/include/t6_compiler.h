// Compiler-behaviour helpers for matching VC100 (VS2010) code generation.
//
// Build environment, not game source.
#pragma once

// VC100 (VS2010) kept some float expressions on the x87 stack and returned them unrounded in ST0 (excess precision,
// e.g. a returned sum of products, or arithmetic on a value returned in ST0). clang does the same only
// with SSE disabled for that function.
//
// Only the x86 builds: the x64 Store build (VS2019) does all float arithmetic in SSE, in float.
#if defined(__i386__)
#define X87_MATH __attribute__((target("no-sse")))
#else
#define X87_MATH
#endif

// The precision of an expression VC100 left on the x87 stack across several operations, written out where the code
// is otherwise SSE: double on x86 (the x87 runs at double precision, control word 0x27F, so double arithmetic in SSE
// gives the same results); float on x64, where VS2019 computes the same expression in float.
#if defined(__i386__)
typedef double x87_t;
#else
typedef float x87_t;
#endif

// A float copied through the x87 stack (fld / fstp), as VC100 (VS2010) sometimes compiles a plain float assignment.
// The difference from a plain copy: a signaling NaN comes out quiet (fld raises invalid and quiets it).
// On a real CPU, that is: the differential harness's emulator (Unicorn) leaves the NaN signaling, so tests
// can't tell this from a plain copy (docs/verification.md, limits). Used where the disassembly shows it.
#define T6_X87_COPY(dst, src) __asm__("flds %1\n\tfstps %0" : "=m"(dst) : "m"(src))

// fsqrt on the x87 stack (x86), sqrtss (x64)
#if defined(__i386__)
static inline x87_t sqrt_x87(x87_t v) { return __builtin_sqrt(v); }
#else
static inline x87_t sqrt_x87(x87_t v) { return __builtin_sqrtf(v); }
#endif
