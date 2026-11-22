# The Microsoft Store build (final target)

Package `38985CA0.CallofDutyBlackOps2PCMS` version **1.1.8.0**, Store id 9NFB5MDCWF47, Xbox title id `7D44318D`,
MSA app id 0000000049407B30 (`appxmanifest.xml`, `MicrosoftGame.config`). Three applications, each launched through the
GDK's `gamelaunchhelper.exe`: `Game` (`t6sp.exe`), `GameMP` (`t6mp.exe`, registered for `ms-xbl-multiplayer` invites)
and `GameZM` (`t6zm.exe`). Capabilities: `internetClient`, `runFullTrust`, `appLicensing`, `unvirtualizedResources`.

## The executables

| | t6mp.exe | t6zm.exe | t6sp.exe |
|---|---|---|---|
| build id | `t6mp_msstore` | `t6zm_msstore` | `t6sp_msstore` |
| sha256 | cf80b805…bc01 | 7152ebe1…7416 | 0404f79c…5c89 |
| linked (UTC) | 2024-12-10 22:23:11 | 2024-12-10 22:10:24 | 2024-12-10 22:23:07 |
| PDB | `t6mp_msstore.pdb` {ABBFB2F5-97CF-4E1D-8D20-67833D167727} age 3 | `t6zm_msstore.pdb` {4E9D993B-5580-423E-A76F-7E858F0933F1} age 2 | `t6sp_msstore.pdb` {2264CC0D-D741-4057-BEA6-0E9EC12D0746} age 3 |
| `.pdata` functions / chained parts | 16,865 / 15,599 | 16,849 / 15,622 | 14,117 / 12,782 |

PDB directory for all three: `d:\workspace\outrun\t6\pc\dev\game\pc\`. "outrun" is the same port workspace as Ghosts'
Store build (`d:\workspace\outrun\iw6-depot`), so the two Store ports share their platform layer and toolchain.

- **x64.** The original PC game (Steam, 2012–2015) and every server are x86. The Store build is a port to x64, so no
  original binary shares its instruction set; matching runs on strings, constants, registrations and the call graph
  (docs/matching.md).
- **No Arxan, no Steam.** `.text` entropy 6.6, a standard MSVC CRT entry (`__security_init_cookie`, then
  `__scrt_common_main_seh`), no Steam imports.
- **No relocations.** The relocation directory is empty: the exe only ever loads at 0x140000000.
- **Toolchain:** linker 14.29, Rich header objects up to cl 19.29.30154 (VS2019 16.11), dynamic VC14 CRT (`MSVCP140`,
  `VCRUNTIME140`, `VCRUNTIME140_1`, UCRT). LTCG: much inlining, split (chained) functions. Small leaves from
  `universal/` such as `ClampChar`, `Com_HashKey`, `ColorIndex` and `I_isdigit` have no standalone body in
  `t6mp.exe` (tools/match/behaviour.py finds none); they are inlined into their callers.
- **No asserts.** Like the Steam exe and the ship server, the Store exe holds none of the assert source paths
  (`c:\t6\code\src\...`); its `I_stricmp` (140471C80) handles a null pointer the way the
  server's does after an ignored assert. The reconstruction compiles asserts out for it (`asserts` in `target.toml`).
- **Platform:** a `.xbld` section records the GDK editions (June 2022 GRDK Update 4 and March 2024 GRDK Update 2);
  `xgameruntime` is loaded at runtime. Imports include `XCurl.dll` and `WINHTTP` (HTTP), `bcrypt`/`CRYPT32`,
  `XAudio2_9`, `XINPUT1_4`, `d3d11`/`dxgi`, `binkw64` and `api-ms-win-core-psm-appnotify` (process lifetime).
- **Sections:** `.text`, `.interpr` (30 KB of code outside `.text`, normal entropy; to be identified), `.rdata`,
  `.data` (61 MB virtual), `.pdata`, `.rodata`, `_RDATA`, `.xbld`, `.rsrc`.

## The upload in R2 (`bo2/DUMP/`)

The package layout (`layout_05042768-….xml`) lists 1,009 entries. Uploaded (540 objects): the three executables,
`gamelaunchhelper.exe`, `XCurl.dll`, `binkw64.dll`, the manifests and logos, `main/english` fonts, 169 sound banks,
126 videos, `zone/all/` (147 entries including `base.ipak`) and `zone/english/` (74 fastfiles). Not uploaded: the
French, German, Italian and Spanish localisation (likely not installed). The code work needs only the executables;
the fastfiles are needed for asset work and for running the game.

## Reconstructing against the Store build

The Store builds are the target; the server is the reference that names things. Differences found so far, and how
the source handles them:

- **Source revision.** `store = true` in `target.toml` compiles with `T6_STORE`. The Store's source is later than
  the 2013 servers': `usercmd_s` is 0x3C bytes (no `gunPitch`, `gunYaw` or Wii U aim fields), and some code
  changed (`CL_KeyMove`'s stance hold times are constants, 300 and 200 ms, where the server reads
  `cl_stanceHoldTime` and `cl_dtpHoldTime`). Code that differs is written for each revision under `T6_STORE`.
- **Layouts.** Structures keep the PDB's field types, so pointers widen on x64 and the Store offsets follow; each
  structure has `static_assert`s for both builds, the Store's taken from its code (the function named beside them).
  `tools/pdb/layout64.py` predicts every PDB type's x64 layout (`analysis/CoDMPServer_PC/pdb/layout_x64/`): it lays
  each type out with MSVC's rules for x86 first, inferring `#pragma pack` and `__declspec(align)` until the PDB's
  offsets come out (4,844 of 4,944 types), then for x64. Where the Store's code disagrees with the prediction, the
  Store revision changed the type: `usercmd_s` (8 bytes shorter), `cg_t` (0x20 bytes more between
  `predictedPlayerState` and `damageKickAngles`), `clientStatic_t` (one int fewer before `frametime`), `demoMain`
  (4 bytes more before `state`), `demoPlayback` (0x80 bytes more before the free camera's lock), `centity_t` (0x3D8
  bytes; `nextState` 0x18 further on) and `AITypeScript` (`demoLockOn` at +0x48).
- **Arithmetic.** The Store build is SSE throughout, in float. Where the server kept a chain on the x87 stack, the
  source uses `x87_t` (double on x86, float on x64) and `X87_MATH` (x86 only): `t6_compiler.h`.
- **Fast floating point.** The Store was compiled with a fast floating-point mode (`/fp:fast`): it reassociates
  and rewrites comparisons, so the same source line can round differently or treat NaN differently.
  - *Reassociation.* `CL_MouseMove`'s acceleration is `(accel / frame_msec) * speed` on the Store and
    `accel * (speed / frame_msec)` on the server.
  - *Comparisons.* A comparison can be swapped or folded in a way only NaN tells apart: `a >= b` as `!(b > a)`
    (`CG_IsSprinting`); `I_fmin(x, 127.0f)` as `x > 127 ? 127 : x` (a NaN stays NaN); `x != 0` as
    `x < 0 || x > 0` (a NaN is zero); `cap <= 0 || 360 - cap < 0` as `!(cap > 0 && cap <= 360)`.
  - The Store's branch writes each such expression the way the Store evaluates it, per site (the compiler didn't
    rewrite every use of a helper the same way), and its test includes the NaN and rounding inputs that tell the two
    apart: a test that passes with the server's form reverted in is missing them.
- **Revision changes in code.** `CL_MouseMove` rounds the mouse's walk and strafe moves with `floorf(x + 0.5f)`
  (the server rounds to nearest after adding 2^-30) and no longer prints `cl_showMouseRate`. `CL_AdjustAngles` has
  fixed values where the server reads dvars (the speed key's 1.5, a default turn speed of 140 degrees a second).
  `CL_CreateCmd` lets the location selector run first, then holds still while the controller's system UI (per
  controller on the Store) or the LUI catcher (0x10) has the keys, without clearing the key states. The demo
  camera's AI lock-on always takes the first view. `Com_LocalClient_LastInput_Set` (1403946A0) also sends LUI an
  event when the input device changes; it isn't reconstructed for the Store yet.
- **The mouse reader.** The Store's `IN_MouseMove` (140499A10) inlines a later revision of `CL_MouseEvent`:
  - a Store-only flag (1427F1B60, name unknown) takes the dev GUI check's place;
  - the 3D-window check is gone;
  - key catcher 0x10 sends the movement to a LUI handler (1406AC5D0).

  The UI cursor functions it inlines (`UI_GetInfo`, `UI_UpdateMouseScreenPosition`, `UI_MouseEvent`) are the
  server's.
- **Inlined wrappers change the callee.** Where the Store inlined a thin wrapper, its callers call what the wrapper
  called: `CL_MouseMove` calls `GamerProfile_GetInt` and uses the bits as a float (the server's
  `GamerProfile_GetFloat`), so the returned value is in EAX, not XMM0. A Store function whose result is used
  unlike its server twin's is a wrapper's callee, not the twin.
- **Inlining.** Link-time code generation inlined most small functions (`CL_GetLocalClientGlobals`, `Dvar_GetInt`,
  `ClampChar`, `CL_AdjustAngles`, ...) and removed unused arguments (`CG_IsSprinting` takes none). A function the
  Store doesn't have is verified there through its caller; header functions are inline in `include/` as they were
  in the original. A Store case can't fake or observe a function the Store inlined.
- **Identification.** `records/names.tsv` rows for the Store come from `tools/match/match.py`, behaviour, or the
  structure of the code (the call order and field accesses of a function whose server twin is reconstructed),
  with the evidence written out.
- **Stack contents.** A local the original reads before writing (`CL_MouseMove`'s `AimOutput` melee fields when aim
  assist skips auto-melee: B0016) reads what's on the stack. The harness fills the top of the stack with a fixed
  pattern before every call, so the original and the reconstruction read the same bytes when neither wrote them.
