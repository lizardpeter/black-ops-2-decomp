# Enhanced build roadmap

The enhanced build is the faithful reconstruction plus deliberate, documented changes (`enhanced/`,
`records/patches.tsv`). The goals match ghosts-decomp's and black-ops-3-decomp's. Each item says what has to be
decompiled first. Items marked **open** still need a decision. Fixes to original behaviour come from
`records/bugs.tsv`.

## Input

- **True raw input: done (P0002).** The mouse's raw input is read on a thread of its own: `WM_INPUT`, device
  counts, 64-bit counters. The game turns by the counts; menus keep the cursor. Absolute devices (tablets, remote
  desktop) are ignored.
- **No acceleration, no smoothing: done (P0002).** `cl_mouseAccel` and `m_filter` are removed from the enhanced
  `CL_MouseMove`, not defaulted off. Raw input bypasses Windows' pointer acceleration.
- **High polling rates and DPI: done for the mouse (P0002).** Each report is handled on the input thread, counts
  are never rounded, and a 0 ms frame no longer drops the frame's movement (B0017). The yaw is kept within ±180° so
  its precision doesn't wear down (B0018). Still to do: keyboard events through raw input, so short presses between
  frames aren't lost.
- **MW2019 sensitivity scale: done (P0003).** `m_sens`: degrees per count = MW2019's yaw (0.0066°) × `m_sens`,
  default 1.5, independent of FOV and ADS; angles kept in double precision. Same setup as Ghosts. Still possible: an
  option to scale ADS by the zoom (MW2019's "monitor distance" coefficient), if wanted.

The enhanced build reaches the game through `runtime/loader` (docs/enhanced-build.md).

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

## Audio

- **WASAPI, low latency.** The game mixes through XAudio2 (`XAudio2_9.dll`), which runs on WASAPI in shared mode
  with the system's default buffer (typically 10 ms). Output through our own WASAPI path instead:
  `IAudioClient3` low-latency shared mode (the smallest period the driver allows, often about 3 ms), with exclusive
  mode as an option. Voice chat capture and playback (DirectSound and WINMM in the original) move to WASAPI too.
- **Leave the system microphone alone (B0002): done, P0001.** The original sets every Windows recording device's level
  to 100% and its mute state when voice starts, and only restores them on a clean exit. The enhanced build never
  touches device settings; `winvoice_mic_mute` mutes the game's own capture. Still to do: map
  `winvoice_mic_reclevel` onto the capture gain once `groupvoice/record.cpp` is reconstructed.

First: the sound back end (`snd_*`, XAudio2 driver layer).

## Threading

BO2 already has threads: render back end, render and FX workers, database (fast file) loading, streaming, and a
server thread. The game simulation, client, UI and most of the front end run on the main thread, and workers sync
with waits each frame (`R_WaitWorkerCmds`, `FX_WaitFor*`).
- Profile first (where the main thread spends a frame, where workers wait).
- Then a job system sized to the CPU's cores, replacing fixed worker counts, and moving independent work off the main
  thread (scene culling, animation, physics, sound updates, asset streaming).
- Data races in the original become ledger rows as they're found.

## Memory: 64-bit and limits

The Store build is 64-bit and already large-address aware, so there's no 4 GB process limit like the 32-bit Steam
build (which only gets 4 GB at all because it's large-address aware on 64-bit Windows). What still limits it are the
engine's own fixed pools: the fast file memory, the image and streaming pools, and 32-bit sizes and offsets inside
its data structures. Those get measured and raised as `limit` rows in the ledger, which also covers higher texture
streaming budgets.

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

  Recommendation: 1 + 2 by default, with 3 as an option to measure. Pace the pipeline like Forge (Rust-test
  `docs/PERFORMANCE_TELEMETRY.md`, "Frame overlap"): start the main thread's frame just in time to be ready when
  the render thread and GPU free up (their expected finish less the main thread's recent frame time), instead of
  letting it run ahead, so a finished frame never waits at the handoff and its input is no older. Still to discuss
  when we get there.
- **Fixed-rate simulation must not stall frames.** When simulation moves to a fixed tick (frame rate independence,
  above), the tick runs on its own schedule with rendering interpolating between ticks, not inside one render frame
  every tick: CS2's 64 Hz client tick on the main thread makes a periodic frame-time spike (Blur Busters analysis,
  Sept 2026).
- **Refresh rates above 240 Hz** and uncapped frame rates without frame-rate coupling (see above).

## Rendering

- **Native Vulkan,** with no translation layers (no DXVK), as decided for Ghosts and BO3. It needs the D3D11
  back end (`gfx_d3d/`) decompiled and its shaders converted. BO3's plan applies: DXBC to SPIR-V first, then
  decompiled shaders one at a time.
- **Resizable BAR:** a hardware and driver feature, not something the game turns on. Under D3D11 the driver decides
  (NVIDIA only enables it for games in its profile list). Under our Vulkan renderer we can use it directly: upload
  dynamic buffers and streamed textures straight into memory that is both device-local and CPU-visible, instead of
  copying through staging buffers.
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
