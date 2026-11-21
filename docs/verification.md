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
- `include/math.h`: the float functions are real functions on x86 (the server calls `___libm_sse2_sinf` directly
  at 90 sites); `atan2f` is VC100's inline double wrapper. On x64 they're UCRT imports.
- `t6_static.h`, `t6_assert.h`, `t6_compiler.h`, and minimal `string.h` / `windows.h`.

## matches: differential tests

`tests/differential/run.py --build <id> [--check-records] [-k name]` maps the original exe and the build's
`recon.dll` into one Unicorn machine (`harness.py`) and runs each exported function and its original on the same
inputs. Cases live in `test_*.py` as `CASES`, keyed by scoped plain name, so one case serves every build. A case
passes when every input gives the same:
- return value (floats bit for bit; an x86 float result is read from ST0 unrounded);
- contents of every buffer passed by pointer;
- bytes written to either image's globals, with pointers to code mapped back to the original function and pointers
  to string literals compared as strings;
- observed calls.

Inputs on which the original itself faults are skipped. Writes are journaled and rolled back, so calls never see each
other's state.

Harness specifics:
- **x86:** cdecl stack arguments (`('q', v)` for a 64-bit integer), FS on a TEB through a GDT descriptor, x87
  control word 0x27F.
- **x64:** Win64 registers and shadow space, GS on a TEB with a PEB.
- **Both:** `rdtsc` is deterministic.
- **Imports:** on x64 an unhooked import returns 0. On x86 one faults, naming the import, until a test registers it
  with `Machine.on_import(name, fn, nargs)`: Win32 imports are stdcall, and a stub that doesn't pop their
  arguments would silently corrupt the stack.
- **Instruction cap:** 20 million per call. It's a runaway guard, and `-O0` reconstructions take several times the
  original's instructions.

`--check-records` fails when `records/functions.tsv` claims `matches` for a function of that build that didn't pass.
CI (`.github/workflows/check.yml`) builds and tests `CoDMPServer_PC`, `t6mp` and `t6mp_msstore` on every push.

## Byte matching

Not attempted: it would need the original compilers (VS2010 for the servers, VS2019 16.11 with LTCG for the Store
build). The tests compare behaviour.
