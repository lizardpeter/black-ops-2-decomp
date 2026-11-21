# x86 builds only (the VS2010 servers). With /arch:SSE2, VC100 compiles calls to sin(), sinf(), ... into calls to
# the CRT's ___libm_sse2_* routines, which take arguments in XMM0/XMM1 and return in XMM0. These cdecl shims give
# the reconstruction the normal C entry points and forward to those exact routines. The routines themselves resolve
# to the ORIGINAL binary's copies through the hybrid link (tools/build_recon.py), so reconstructed and original code
# run the same math library bit for bit. Only the routines CoDMPServer_PC links are forwarded: double asin, atan2,
# cos, exp, log, pow, sin, tan; float acosf, asinf, atanf, cosf, expf, logf, log10f, powf, sinf, tanf.
	.intel_syntax noprefix
	.text

.macro UNARY name
	.globl _\name
_\name:
	movsd xmm0, qword ptr [esp + 4]
	call ___libm_sse2_\name
	sub esp, 8
	movsd qword ptr [esp], xmm0
	fld qword ptr [esp]
	add esp, 8
	ret
.endm

.macro BINARY name
	.globl _\name
_\name:
	movsd xmm0, qword ptr [esp + 4]
	movsd xmm1, qword ptr [esp + 12]
	call ___libm_sse2_\name
	sub esp, 8
	movsd qword ptr [esp], xmm0
	fld qword ptr [esp]
	add esp, 8
	ret
.endm

# float routines: argument(s) in XMM0 (XMM1) as singles, result in XMM0 as a single; cdecl returns it in ST0
.macro UNARYF name
	.globl _\name
_\name:
	movss xmm0, dword ptr [esp + 4]
	call ___libm_sse2_\name
	sub esp, 4
	movss dword ptr [esp], xmm0
	fld dword ptr [esp]
	add esp, 4
	ret
.endm

.macro BINARYF name
	.globl _\name
_\name:
	movss xmm0, dword ptr [esp + 4]
	movss xmm1, dword ptr [esp + 8]
	call ___libm_sse2_\name
	sub esp, 4
	movss dword ptr [esp], xmm0
	fld dword ptr [esp]
	add esp, 4
	ret
.endm

	UNARY asin
	UNARY cos
	UNARY sin
	UNARY tan
	UNARY exp
	UNARY log
	BINARY atan2
	BINARY pow

	UNARYF acosf
	UNARYF asinf
	UNARYF atanf
	UNARYF cosf
	UNARYF expf
	UNARYF logf
	UNARYF log10f
	UNARYF sinf
	UNARYF tanf
	BINARYF powf
