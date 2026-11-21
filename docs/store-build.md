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
- **Platform:** a `.xbld` section records the GDK editions (June 2022 GRDK Update 4 and March 2024 GRDK Update 2);
  `xgameruntime` is loaded at runtime. Imports include `XCurl.dll` and `WINHTTP` (HTTP), `bcrypt`/`CRYPT32`,
  `XAudio2_9`, `XINPUT1_4`, `d3d11`/`dxgi`, `binkw64` and `api-ms-win-core-psm-appnotify` (process lifetime).
- **Sections:** `.text`, `.interpr` (30 KB of code outside `.text`, normal entropy; to be identified), `.rdata`,
  `.data` (61 MB virtual), `.pdata`, `.rodata`, `_RDATA`, `.xbld`, `.rsrc`.

## The upload in R2 (`bo2/DUMP/`)

The package layout (`layout_05042768-….xml`) lists 1,009 entries. Uploaded so far: the three executables,
`gamelaunchhelper.exe`, `XCurl.dll`, `binkw64.dll`, the manifests and logos, `main/english` fonts, 169 sound banks and
98 videos. Not yet uploaded: `zone/all/` (147 entries including `base.ipak`) and `zone/english/` (74 fastfiles), 28
videos, and the French, German, Italian and Spanish localisation (likely not installed). The code work needs only the
executables; the fastfiles are needed for asset work and for running the game.
