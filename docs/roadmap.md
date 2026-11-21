# Enhanced build roadmap

The enhanced build is the faithful reconstruction plus deliberate, documented changes (`enhanced/`,
`records/patches.tsv`). The goals match ghosts-decomp's and black-ops-3-decomp's. Each item says what has to be
decompiled first. Items marked **open** still need a decision. Fixes to original behaviour come from
`records/bugs.tsv`.

## Input

- **True raw input.** Read mouse movement from `WM_INPUT` / `GetRawInputBuffer` as relative counts, with no
  cursor centering or `ClipCursor` box. The original moves the Windows cursor and reads it back
  (win32 input: `IN_MouseMove` and friends), which loses, clamps and accelerates counts. The menus keep an absolute
  cursor.
- **High polling rates, mice and keyboards.** Drain the whole raw-input buffer every frame (8 kHz mice deliver
  several events per millisecond). Keyboard events come through raw input too, so short presses between frames
  aren't lost.
- **Full DPI support.** Accumulate counts unrounded, in double, and apply them once per command. No integer
  truncation of small movements.
- **MW2019 sensitivity scale.** Degrees per count = MW2019's yaw (0.0066°) × sensitivity, independent of FOV unless
  a scale option asks for it. Same setup as Ghosts and BO3.

First: `client/cl_input.cpp` (`CL_MouseMove`, `CL_CreateNewCommands`), `win32` input, `cgame` view angles.

## View

- **FOV unlocked** (`cg_fov` beyond the original's range) and **FOV scale unlocked** (`cg_fovScale`). Check
  Hor+ on wide and ultrawide aspect ratios, and how weapons, viewmodel and UI follow.

First: `cgame_mp/cg_view*.cpp` (`CG_CalcFov` and callers).

## Frame rate independence

Everything that runs per frame and uses frame time is a candidate: player movement (`Pmove` / `PmoveSingle`; in older games of this engine
family, frame-time rounding changed jump height and strafe speed at high FPS, to check in BO2), physics, animation and
effects timing, menus and HUD lerps, and command generation.
- **Simulation:** fixed steps, or exact accumulation of frame time.
- **Presentation:** interpolated from the simulation.
- **Behaviour players rely on:** records it in `records/bugs.tsv` with `keep`, so it stays.

Raise the `com_maxfps` ceiling (1000 in the original).

## The "connection interrupted" warning (B0001)

Found and recorded: the warning counts commands, not time. `CG_DrawDisconnect` shows it when the usercmd from 127
commands ago is newer than the last acknowledged one. With one command per frame the window is 127 frames: about
1 s at 125 FPS, 127 ms at 1000 FPS. During kill cams the acknowledged time stops moving, so it shows even sooner.
That's why it plagues high frame rates and kill cams.

Fix in two parts:
- **Measure the right thing:** time since the last acknowledgement against a real threshold.
- **Decouple command generation from frame rate:** a fixed command rate with input sampled at the latest moment
  (late latch). Then the 128-command buffer covers a fixed span of time, and the server doesn't receive one command
  per rendered frame.

Before changing anything, confirm whether a wrapped buffer also costs real command redundancy in `CL_WritePacket`
(the "interrupted" state may not be purely cosmetic).

First: `cgame_mp/cg_draw_net_mp.cpp`, `client/cl_input.cpp`, `client_mp/cl_input_mp.cpp`.

## Latency and presentation

- **Hardware independent flip:** a DXGI flip-model swap chain (`FLIP_DISCARD`), tearing allowed and a waitable
  object. Borderless then gets independent flip with no compositor frame. No V-Sync by default.
- **Late latch:** sample view angles as late as possible, at render submit, as in Ghosts and BO3.
- **NVIDIA Reflex:** latency markers plus the Reflex sleep, as a menu option.
- **Frames in flight (open):** BO2 pipelines frames across threads. The main thread builds frame N+1 while the render
  thread submits frame N (`R_ToggleSmpFrame`, `Sys_WakeRenderer` / `Sys_WaitRenderer`), and the GPU may queue
  more. Options:
  1. Keep the two-thread pipeline and cap the GPU queue at 1 (waitable swap chain, maximum frame latency 1).
     Late latch removes most of the pipeline's input delay, because the newest angles reach the frame at submit.
  2. Reflex: it sleeps the CPU so the queue stays empty without losing GPU throughput.
  3. Run the back end on the main thread: lowest latency, highest frame time.

  Recommendation: 1 + 2 by default, with 3 as an option to measure.
- **Refresh rates above 240 Hz** and uncapped frame rates without frame-rate coupling (see above).

## Rendering

- **Native Vulkan,** with no translation layers (no DXVK), as decided for Ghosts and BO3. It needs the D3D11
  back end (`gfx_d3d/`) decompiled and its shaders converted. BO3's plan applies: DXBC to SPIR-V first, then
  decompiled shaders one at a time.
- **HDR:** scRGB or HDR10 swap chain, tone mapping in post, and UI composited at a set paper white.
- **Optimisations** from profiling and the bug ledger's `performance` rows.

## Platform, services and external DLLs

- **Official servers cut,** as for Ghosts and BO3: no Demonware, Xbox Live or Store traffic. The engine's own UDP
  networking stays, for peer/LAN play and our own server later.
- **External DLLs:** replace or reimplement them natively where they tie the game to vendors.
  - `XCurl.dll`: HTTP to vendor services, cut.
  - `xgameruntime.dll` (GDK): offline implementation, as BO3.
  - `binkw64.dll`: Bink video, keep or replace.
  - XAudio2, XInput and D3D11 are Windows components.
- **Discord integration:** rich presence over Discord's local IPC pipe (`\\.\pipe\discord-ipc-0`, JSON frames),
  implemented natively, with no Discord SDK DLL.
- **COD Elite-style class editing (open):** loadouts are player stats (DDL-defined). With our own server storing
  stats, a web or companion API can read and edit classes outside the game, as Elite did. That needs the server and
  the stats/DDL code first.

## Security

Fix the known remote code execution and crash bugs the community clients patch, confirmed against our
reconstruction and written up in `docs/security.md`. Harden every network and file parser (security-critical
functions make good early decomp targets).

## One executable for MP, ZM and campaign (open)

Feasible after a full source-level decomp; not by merging the shipped binaries.
- **What's shared:** the three exes share the engine (renderer, sound, file system, network, script VM).
- **What differs:** they compile the game code differently. Zombies is the multiplayer code base with zombies
  defines; the campaign is a different game module (`game/`, actors, save games). Shared structures (entities,
  player state) have mode-specific layouts.
- **How:** build each mode's game code as its own module behind one interface, chosen at startup or from the main
  menu, as BO3 ships MP/ZM/CP in one exe. It costs the most while decompiling (three game modules to reconstruct
  and verify). It's natural in the Rust rewrite, where the module boundary can be designed in.
- **Order:** finish MP, then ZM (closest to MP), then campaign.

## Rust rewrite

Last, from the verified C++ reconstruction: one subsystem at a time behind the same interfaces, each tested
differentially against the C++ version. Same approach as Ghosts.

## Also worth doing (suggestions)

- **Server tick and snapshot rates:** measure BO2's server frame and snapshot rates (`sv_fps` and snapshot code in
  `server_mp/`). Make the tick rate configurable on our own server, with lag compensation tuned to it.
- **Mod support:** custom GSC and maps, loaded from the game folder, as Plutonium does.
- **Raised asset and engine limits** (fastfile, entity and string table caps), recorded as `limit` rows.
- **Ultrawide and UI scaling:** HUD anchoring, safe areas, menus at any aspect ratio.
- **Theater and demos** recorded and played back at any frame rate.
- **Offline progression:** stats and unlocks kept locally (or on our server) without Demonware.
- **Stutter fixes:** shader and asset streaming hitches measured and fixed.
