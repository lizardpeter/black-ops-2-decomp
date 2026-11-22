// Static data in the hybrid build.
//
// Build environment, not game source. A `static` variable in the original (file scope or inside a
// function) lives at a fixed address in the original binary, but it has no public symbol, so it can't
// be linked by name like globals can. While the reconstruction is tested against the original, it
// must read and write that same storage. So reconstructed code declares original statics like this:
//
//     #define T6_SOURCE "universal/com_math.cpp"   // once per file: the PDB source path
//
//     T6_STATIC unsigned int holdrand T6_AT("", "holdrand");
//     T6_STATIC const float sRGBtoXYZ[4][4] T6_AT("", "sRGBtoXYZ") T6_INIT(= { ... });
//
//     int BoxOnPlaneSide(...)
//     {
//         T6_STATIC int bops_initialized T6_AT("BoxOnPlaneSide", "bops_initialized");
//
// T6_AT(scope, name): scope is "" for a file static, or the enclosing function's name.
// tools/build_recon.py resolves every such symbol to the address in analysis/<build>/pdb/statics.tsv.
// T6_INIT keeps the original initializer readable in the source; in the hybrid build the original
// binary's storage already holds it.
#pragma once

#define T6_STATIC extern
#define T6_AT(scope, name) __asm__("t6s@" T6_SOURCE "@" scope "@" name)
#define T6_INIT(...)

// A global (public) variable the reconstructed file defines in the original. While the build is hybrid-linked its
// storage stays in the original binary, found by its decorated name in the MAP; T6_INIT keeps the initializer in the
// source:
//
//     T6_GLOBAL const vec4_t colorBlack T6_INIT(= { 0.0f, 0.0f, 0.0f, 1.0f });
#define T6_GLOBAL extern

// A thread-local original variable (`__declspec(thread)`), declared with T6_STATIC / T6_GLOBAL:
//
//     T6_STATIC T6_THREAD void **g_dwTlsIndex T6_AT("", "g_dwTlsIndex");
//
// tools/build_recon.py places it in the DLL's .tls at its offset in the original's TLS block. The original's
// variables are constant-initialized; saying so keeps clang (as MSVC 2019 for an extern thread_local) from guarding
// each access with the on-demand dynamic TLS initialization (__tls_guard, __dyn_tls_on_demand_init).
#define T6_THREAD __declspec(thread) __attribute__((require_constant_initialization))
