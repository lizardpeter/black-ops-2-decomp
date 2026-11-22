# Verification

## compiles: the hybrid-link build

`tools/build_recon.py --build <id>` compiles every `.cpp` under `src/` with clang for the build's architecture
(`target.toml`) and links one Windows DLL, `build/<id>/recon.dll`. Anything the reconstruction uses but doesn't
define yet resolves to the original exe through `tools/symbols.py`. So a function can be reconstructed and tested
before anything it calls is; `build/<id>/hybrid.tsv` lists each such dependency.

| | x86: `CoDMPServer_PC` (and the other servers), `t6mp` (Steam) | x64: `t6mp_msstore` (Store) |
|---|---|---|
| Compiler emulated | VS2010 (`-fms-compatibility-version=16.00`), `/arch:SSE2` | VS2019 16.11 (`19.29`) |
| Names resolve through | the MAP (decorated) and PDB statics (`T6_STATIC`) | `records/names.tsv` and the matcher's pairs, by scoped plain name |
| Original function | thunk `push va; ret` | thunk `jmp [rip+slot]` |
| Original data | absolute symbol | `movabsq` rewritten to a slot load (`-mcmodel=large`) |
| Not identified yet | build error for a MAP build; `ud2` trap for the Steam exe | `ud2` trap, or a data slot holding 0 |
| DLL base | fixed 0x10000000 | 0x180000000, ASLR; slots rebased at load by `t6_hybrid_entry` |

The x64 slot table comes from black-ops-3-decomp (its docs/hybrid-link.md). BO2's Store exe has no relocations, so
it always loads at 0x140000000, but the table makes the DLL independent of where either image sits. The link fails
if any code holds an exe address directly.

Source is C++ with the original prototypes. For a build with a MAP, a function whose decorated name differs from the
original's (parameter types, constness, calling convention) stops the build, since it would otherwise silently drop
out of testing. One source serves both architectures: the Store build's x64 names differ from the servers' x86
ones, which is why the Store build resolves through scoped plain names. `-fno-threadsafe-statics` keeps x64 statics
as VS2010 initialised them.

Build environment (`tools/recon/`), not game source:
- `crt_sse2.s` (x86): VC100 compiles `sinf`, `cos`, ... into calls to the CRT's `___libm_sse2_*` routines
  (arguments and result in XMM registers). These shims forward the C entry points to the original's copies, so both
  sides run the same math library. Only the routines CoDMPServer_PC links are forwarded.
- `crt_x64.s` (x64): `strlen` and `strcmp`, which VS2019 expands inline, so the Store code never calls the CRT's
  (and the harness stubs imports), and `__chkstk`, the CRT's stack probe clang calls for frames over a page.
- `include/math.h`: the float functions are real functions on x86 (the server calls `___libm_sse2_sinf` directly
  at 90 sites); `atan2f` is VC100's inline double wrapper. On x64 they're UCRT imports. `sqrtf` and `fabsf` are
  intrinsics (`sqrtss` / `andps`, or `fsqrt` / `fabs` in x87 code), and `-fno-math-errno` keeps them inline.
- `t6_static.h`: `T6_STATIC` for a static variable (`T6_AT(scope, name)` finds it in the PDB's statics) and
  `T6_GLOBAL` for a global the file defines; both stay in the original binary while hybrid-linked, with `T6_INIT`
  keeping the initializer in the source.
- `t6_assert.h`: one macro per argument shape the original's asserts take (the line is the original's). Only builds
  with `asserts = true` in `target.toml` keep them (CoDMPServer_PC, the only one whose exe holds the assert source
  paths); for the others `build_recon.py` defines `T6_NO_ASSERTS` and they compile away, expression and all, as in
  the retail and ship builds. A header's asserts name it as the including file saw it (`client_mp\../client/client.h`),
  which is the same for every copy of a header function, so headers write that path out (`assert_in`, `assertf_in`).
- `t6_compiler.h`: `X87_MATH` for a function VC100 kept on the x87 stack (excess precision), `T6_X87_COPY`.
- Minimal `string.h` (`strcpy` / `strcat` as VC100's inline forward-copy expansions: the servers have no
  out-of-line copy), `stdio.h`, `stdlib.h`, `ctype.h`, `setjmp.h`, `windows.h`.

Writing for the reconstruction's compiler: at `-O0` clang returns through a stack slot when a function has several
`return`s, and spills any x87 value that crosses a branch, both as float. Either rounds a result VC100 returned
unrounded in ST0, so an `X87_MATH` function computes its result in the block that returns it (q_shared.cpp
`UnGetLeanFraction`, `CMD_GetAnalogButtonValue`). It can't return an int converted on the x87 stack (`fild`)
unrounded from a function with several returns (`Dvar_GetFloat`); such inputs are compared on the Store only. A value the original kept on the x87 stack across a call has to be
computed after the call: clang spills it as a float otherwise (cl_input.cpp `CL_AdjustAngles` reads the dvar first). Likewise a store the original's optimiser removed is left out when
keeping it would change behaviour (q_shared.cpp `Info_RemoveKey_Big`: the dead value copy would overflow the stack).

Custom calling conventions: the server's link-time code generation passed many functions' arguments in registers
(EAX, ECX, EDX, ESI, EDI, XMM0) instead of on the stack. `records/conventions.tsv` (authored) lists them per build:
each argument's register or `stack`, with the evidence from the disassembly. `tools/conventions.py` helps find them:
it computes each function's live-in registers into `analysis/<build>/conventions.tsv`. The reconstruction keeps the
standard convention, so `build_recon.py` reaches such an original through an adapter thunk (saves EBX/ESI/EDI, loads
the register arguments, pushes the stack ones, calls, restores), and the tests call the original with its convention.

## matches: differential tests

`tests/differential/run.py --build <id> [--check-records] [-k text]` (text in the function's name or source file) maps the original exe and the build's
`recon.dll` into one Unicorn machine (`harness.py`) and runs each exported function and its original on the same
inputs. Cases live in `test_*.py` as `CASES`, keyed by scoped plain name, so one case serves every build. A case
passes when every input gives the same:
- return value (floats bit for bit; an x86 float result is read from ST0 unrounded);
- contents of every buffer passed by pointer;
- bytes written to either image's globals, with pointers to code mapped back to the original function and pointers
  to string literals compared as strings;
- observed calls.

Inputs on which the original itself faults are skipped. A reconstruction that runs into a hybrid-link trap (a
function the build doesn't identify yet) is reported `blocked`, not failed. Writes are journaled and rolled back, so calls never see each
other's state. A case that fakes a function the build doesn't identify yet is reported `blocked` instead of run (the
original would call the real one); hooks on `Assert_MyHandler` are dropped for builds without asserts.

Harness specifics:
- **x86:** cdecl stack arguments (`('q', v)` for a 64-bit integer), FS on a TEB through a GDT descriptor, x87
  control word 0x27F.
- **x64:** Win64 registers and shadow space, GS on a TEB with a PEB.
- **Both:** `rdtsc` is deterministic. The top 64 KB of the stack is filled with `0xCC` before every call, so a local
  read before it is written (an original bug, e.g. B0016) gives the same bytes in both runs instead of whatever the
  previous call left. A failing case prints the failing input's index, the differing writes and buffers with both
  runs' bytes, and the first differing observed calls.
- **Imports:** an import with no model faults, naming it, until a test registers one with
  `Machine.on_import(name, fn, nargs)`. On x86, Win32 imports are stdcall, and a stub that doesn't pop their
  arguments would silently corrupt the stack. On x64 the harness models the CRT calls the Store build makes
  (`CRT_MODELS_X64`: `memset`, `memcpy`, `memmove`, `strchr`, `strlen`, `atoi`), since the reconstruction calls
  what the Store inlines (CL_CreateCmd's `memset`) and some Store code calls them itself (Info_RemoveKey_Big's
  `strchr`). Before, an unmodelled x64 import returned 0 in both runs, which hid both. The models that write
  (`memset`, `memcpy`, `memmove`) write as the program would, journaled and compared: before, a copy into compared
  memory made through the import was invisible to the comparison (found by a missed mutation in `RB_RenderThread`).
- **Functions that never return** (a thread's loop): a fake or an import model calls `Machine.end_call()` and the
  run stops there as if it had returned; the test scripts how far both runs go (`RB_RenderThread`).
- **`T6_TRACE=<n>`** prints the last n instruction addresses of a run that faults.
- **CRT per-thread data (x86 with a MAP):** VC100's CRT builds it on first use through FLS/TLS imports the harness
  doesn't model, so `_getptd` and `_getptd_noexit` return one ready `_tiddata` with the process's default locale
  (`Machine.crt_thread_data`). `_vsnprintf` needs it to format numbers.
- **Thread-local storage:** the TEB's TLS array (fs:[0x2C], gs:[0x58]) leads to a copy of the original's TLS
  template. Reconstructed code reaches the same block: `tools/build_recon.py` links the original's TLS index variable
  and lays out the DLL's own `.tls` so each original TLS variable (`T6_THREAD`, e.g. threads.cpp's `g_dwTlsIndex`)
  has its original offset. TLS writes are compared too.
- **COM objects** (`tests/differential/com.py`, from ghosts-decomp, x86 and x64): fake Direct3D/DXGI objects whose
  every method call is logged with its arguments and compared; writes through out parameters are journaled.
- **Program objects:** data a global pointer points to (the heap-allocated `clients` array) goes in the `OBJECTS`
  region, whose writes are compared like the images' globals.
- **Observed calls:** the hooked function and pointer-sized arguments (`q`) that point into the reconstruction are
  mapped back to the original's addresses; integers (`i`) are compared as they are, even inside the DLL's range.
- **Conventions:** originals in `records/conventions.tsv` are called with their registers; reconstructed `__thiscall`
  members get `this` in ECX.
- **Float results:** a fake returning a Python float leaves it in ST0 on x86 (through a `fld; ret` stub), XMM0 on
  x64.
- **Per-build cases:** a case's `by_build` replaces its fakes or observed calls for one build (what the Store
  inlined can't be faked or observed there); an input with `builds` runs only on those builds, with a comment saying
  why. A fake blocks a case only when the reconstruction calls the faked function in that build (`hybrid.tsv`).
- **Standing fakes:** `Machine.provide(va, fn)` replaces a function in every call; a case's `fakes` apply to it alone.
- **Instruction cap:** 20 million per call. It's a runaway guard, and `-O0` reconstructions take several times the
  original's instructions.

`--check-records` fails when `records/functions.tsv` claims `matches` for a function of that build that didn't pass.
CI (`.github/workflows/check.yml`) builds and tests `CoDMPServer_PC`, `t6mp` and `t6mp_msstore` on every push.

## The enhanced build

`tools/build_recon.py --enhanced` compiles `enhanced/` ahead of `src/`; a function defined in `enhanced/` replaces
the faithful one (`/force:multiple` keeps the first definition). Each override belongs to a patch in
`records/patches.tsv`, and the patch's tests say what it promises:
- `tests/differential/run.py --enhanced` runs the faithful cases on the enhanced DLL: every function it doesn't
  override must still match the original (overrides are reported `patched`).
- `tests/enhanced/run.py` runs each patch's `CHECKS` (`tests/enhanced/test_*.py`): usually the original and the
  enhanced function on the same inputs, compared on everything except what the patch changes, plus the change
  itself. P0001 (`test_voice.py`) reuses `test_win_voice.py`'s Windows mixer model: no winmm call, no mixer change,
  everything else as the original.

CI builds and checks the enhanced build for CoDMPServer_PC and t6mp_msstore, and packages the Store one for the
game (docs/enhanced-build.md).

The enhanced Store DLL is the one loaded into the game, so its link differs (tools/runtime_link.py):
- **Exe first.** A function `src/` defines that the Store build identifies becomes a thunk into the exe's copy,
  linked ahead of `src/` (`/force:multiple` keeps the first). The enhanced build compiles one section per function,
  so the losing copies are dropped.
- **Redirect table.** The replacements are exported as `t6_redirects`.
- **Two checks.** No exe branch lands in a replaced function's first 14 bytes, and no trap is reachable from a
  replacement.
- **Uncovered patches stay off the Store.** An enhanced file whose Store side isn't verified stays server-only
  (`#ifndef T6_STORE`: P0001's voice changes reach Store globals not identified yet).
- **Win32 imports.** Win32 calls the Store exe doesn't import (raw input) are real imports of the DLL, from
  `tools/recon/lib/*.def`; the harness stubs them like the exe's.
- **Testing the result.** `tests/differential/run.py --enhanced` on the Store DLL compares the exe with itself for
  every thunked function. The patch's own checks (`tests/enhanced/test_mouse.py`) compare the replacements with the
  original Store functions, and run the exe's entry points with the loader's jumps applied. Unicorn keeps code it
  has translated, so the check drops its cache for the patched bytes (`ctl_remove_cache`).

x64 Win32 calls: `windows.h` declares Win32 functions `dllimport`, so code calls through `__imp_Name`. The x64 link
resolves each `__imp_Name` to the exe's import address table entry, as exe data reached through its slot. Before
this, `__imp_` names linked to traps, and no Store test had reached a Win32 call.

## Byte matching

Not attempted: it would need the original compilers (VS2010 for the servers, VS2019 16.11 with LTCG for the Store
build). The tests compare behaviour.
