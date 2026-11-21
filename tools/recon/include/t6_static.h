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
