# The enhanced build in the game

The enhanced Store build loads into the original Store 1.1.8.0 executables. Multiplayer uses
`t6_enhanced.dll` and Zombies uses `t6_enhanced_zm.dll`. Both use the same `XINPUT1_4.dll` loader.

## Multiplayer enhancements

**P0002, mouse input** (`records/patches.tsv`):
- **Raw input.** The view turns by the mouse's own counts, read on a thread of their own from Windows' raw input.
  The original turned by how far the Windows cursor moved, so Windows' pointer speed and "Enhance pointer
  precision" acceleration applied, and a fast flick could be cut short at the screen's edge (B0015).
- **No acceleration, no smoothing.** `cl_mouseAccel` and `m_filter` do nothing.
- **High DPI and polling rates.** Counts are added up in 64-bit counters, never rounded, and each report is handled
  on the input thread, not the game's, so an 8 kHz mouse costs the game nothing. Nothing is lost between frames.
- **High frame rates.** A frame the engine counts as 0 ms keeps the mouse's movement for the next frame; the original
  dropped it (B0017).
- **Long sessions.** The yaw is kept within ±180°, so small movements stay exact however much you've turned
  (B0018).

**P0003, MW2019 sensitivity:**
- **`m_sens`, in MW2019's units.** One count turns the view by `m_sens` × 0.0066 degrees: the same number gives the
  same cm/360 as in Modern Warfare (2019). The default is 1.5 (about 29 cm per 360° at 3200 DPI); the range is 0.01
  to 100, and it's saved in the config like any archived setting (`seta m_sens 1.5`, or `/m_sens 1.5` in the
  console).
- **The same at any field of view.** Hip fire and aiming down sights turn alike; the FOV and ADS scales don't apply.
  The menu's mouse sensitivity slider no longer does anything; set `m_sens` instead.
- **Inverted look** follows the menu's option (`m_pitch`'s sign), as before.
- **Exact over many frames.** The view angles are kept in double precision between frames, so a movement split over
  more frames (higher polling and frame rates) ends at the same angle.
- The moves the mouse still makes (with +strafe, or with free look off) and the commands' mouse fields use the counts
  scaled as the original's would be at `m_sens` × 0.3 profile sensitivity.

**P0004, key timing:**
- **Keys on the game's clock.** The original timed key presses by the Windows message, on a clock counting from
  when the PC booted, and measured them against the frame's clock counting from when the game started. So a
  movement key's press frame counted for nothing and its release frame counted in full (B0019). Now each key
  event carries the time the raw input thread saw the key change, on the game's own clock, to the millisecond
  (Windows' message times only move in 15.6 ms steps, B0020). Key bindings, typing and menus work as before: the
  keys still reach the game through its window; only their times change.
- **Precise timer kept.** The game asks Windows for a 1 ms timer; Windows 11 drops that for windows that are
  minimized or covered unless the game opts out. It does now.

**P0005, presentation:**
- **Flip model.** The swap chain is `DXGI_SWAP_EFFECT_FLIP_DISCARD` instead of the original's blit model: in
  fullscreen, and in a borderless window covering the monitor, Windows hands the game's frames straight to the
  display (independent flip) instead of copying them through the desktop compositor.
- **At most one frame queued.** The original let the game run up to three frames ahead of the display (DXGI's
  default queue) plus the one on the render thread. Now Present waits once one frame is queued
  (`IDXGIDevice1::SetMaximumFrameLatency(1)`), so input reaches the screen sooner when the GPU is the limit.
- **Tearing without v-sync.** With `r_vsync 0`, a windowed game presents with tearing allowed (where Windows supports
  it), so the frame rate isn't held to the display's and variable refresh rate works in a window too. Exclusive
  fullscreen already tears without v-sync.
- **Resizing keeps working.** The render thread resizes the buffers with the swap chain's own flags (B0025).
- A system that can't create the flip-model swap chain gets the original one.

**P0009, one frame on the CPU:**
- **Input read after the render thread is done.** The original lets the main thread read input and build the next
  frame while the render thread is still submitting the last one: two frames on the CPU, and when the render thread
  is the slower of the two, input read up to a frame earlier than needed. Now each frame waits for the render
  thread before it reads input, so a frame's input, simulation and submission run back to back, and only the GPU
  overlaps them.
- **`r_renderOverlap 1`** brings back the original's overlap: a higher frame-rate ceiling when the CPU is the limit,
  at up to a frame more input latency. It's saved in the config.
- **`com_frameTiming 1`** measures where each frame's time goes and writes it once a second to
  `t6_frame_timing.log` next to `t6_enhanced.dll` (the Store build has no console output): frames per second, and the
  average and maximum in milliseconds of the main thread's own work, its waits (for the render thread, the display
  and the worker jobs), and the render thread's frame (upload, back end, present) and idle time. `com_frameTiming 0`
  stops it; it isn't saved, and off it costs nothing measurable.

**P0008, an idle render thread:**
- Between frames the render thread waits for its next piece of work instead of spinning on a core with `Sleep(0)`
  (B0024), leaving that CPU time to the main and worker threads that build the next frame. Work still starts within
  a millisecond of when the original would have started it.

**P0007, input read when the display has room:**
- The frame waits on the swap chain's frame-latency object (maximum latency 1) after the server frame and before the
  client reads its keys (the second event loop) and mouse (at draw time). So when the GPU or the display is the
  limit, the frame's input is read just before it can be shown, instead of up to a frame or more early while the
  previous frames queue. The render thread counts its presents and the main thread waits once per present; frames
  that aren't presented aren't waited for, and a wait that times out (two refresh periods, 50 ms to 1 s) is
  recovered from on the next frame.
- With v-sync off and no frame cap, the GPU's own pace now decides the frame rate instead of a queue of frames.

**P0006, frame rate cap:**
- **Exact caps.** The original capped the frame rate in whole milliseconds (1000 / `com_maxfps`, rounded down): 144
  ran at 166 fps, 165 at 166, 240 at 250, 360 at 333 (B0026). Now frames start exactly 1 / `com_maxfps` seconds
  apart on the performance counter, so the cap is the number you set. A server's `sv_clientFpsLimit` still caps it.
- **Precise, cheap waiting.** The wait sleeps on a high-resolution timer and spins only the last half millisecond, so
  a frame starts within microseconds of its time; the original polled with `Sleep(1)` and woke up to a millisecond or
  more late.
- **No catch-up bursts.** A frame that runs more than a frame late starts the cadence again from now.
- Still at most 1000 fps: the engine counts game time in whole milliseconds (the 5 ms simulation tick will lift
  that, docs/roadmap.md).

Menus keep the normal cursor. Turn limits in vehicles and on turrets, aim assist's melee lunge and everything else
are the original's.

Limits:
- Absolute-position devices (pen tablets, and mice inside remote desktop or some virtual machines) send positions,
  not movement, and are ignored.
- If Windows refuses raw input (it practically never does), the mouse doesn't turn the view. The log says so.

## Installing

1. Build: `python3 tools/build_runtime.py` (needs the pinned `t6mp_msstore` exe, like the tests). It writes
   `build/t6mp_msstore/runtime/XINPUT1_4.dll` and `t6_enhanced.dll`. CI builds the same two files as the
   `enhanced-runtime` artifact.
2. Copy both into a complete Store 1.1.8.0 installation, next to `t6mp.exe`. The English multiplayer menu
   uses `localization_en.txt`, the startup fastfiles, `zone/all/base.ipak` for the artwork, and
   `zone/english/en_base.ipak` for the menu text.
3. Start multiplayer as usual. `t6_enhanced.log` appears next to the DLLs and should end with
   `enhanced build active`.

To remove it, delete `XINPUT1_4.dll` and `t6_enhanced.dll` (and the log).

## Opening Custom Games locally

The `--custom-games` shortcut applies to multiplayer. Zombies uses the native menu with the local
Demonware service described below.

With the two DLLs in a complete Store multiplayer installation, launch from the game folder:

```powershell
.\t6mp.exe --custom-games
```

The DLL opens the game's **Custom Games** lobby and creates a local private party for the first controller.
Use the lobby's **Setup Game** and **Start Match** controls. The game initializes its default stats for a new
offline profile so the class selection screen can read weapon and loadout data.

## How it gets in

- **The loader** (`runtime/loader`) is `XINPUT1_4.dll`. Both executables import XInput by ordinal (2, 3, 4), and Windows
  looks in the game folder first. The loader passes every XInput call to the system's `xinput1_4.dll`, so
  controllers work as before.
- **Executable detection.** The loader uses the PE timestamp and image size to select the matching enhanced DLL
  and read its `t6_redirects` table. An unsupported executable named `t6mp.exe` or `t6zm.exe` stops during loading.
  In `t6sp.exe` and other applications the loader forwards XInput to the system.
- **Startup checks.** The loader checks the table's executable identity, length, and every function's original bytes
  before writing the 14-byte jumps. MP and Zombies exit with a DLL initialization error if the enhanced DLL is
  missing, incompatible, or any jump fails to install. Details go to the debugger and, when available,
  `t6_enhanced.log` or `t6_enhanced_zm.log` next to the DLLs. Install the updated proxy with its matching enhanced DLL.
- **The DLL calls the exe's own code** (`tools/runtime_link.py`). A function the Store build identifies runs the
  exe's copy, not the reconstruction's. The DLL's own code is only the replacements and what the Store build
  inlines into the functions they replace, which is verified through those functions.

Checks before anything ships (the build fails otherwise):
- **Patch sites:** each replaced function is at least 14 bytes long (`.pdata`), and no branch in the exe lands in
  its first 14 bytes.
- **No traps:** nothing reachable from a replacement is something the Store build doesn't identify yet. That would
  crash in the game rather than fail a test. This follows calls, jumps, and references to functions such as the
  input thread's window procedure.
- **The package:** `tools/build_runtime.py` checks the pinned executable, the loader's XInput ordinals, and each
  table entry's address, bytes and target.
- **Behaviour:** `tests/enhanced/test_mouse.py` covers each change against the original Store functions, and runs
  the exe's own patched entry points in the emulator.

What isn't covered: the loader and the input thread's Windows calls (thread, message window, raw input
registration) run only on Windows. Their logic around those calls (the window procedure, the counters) is tested
in the emulator.

## Offline service patches

The pinned Store multiplayer build applies the offline patches through the regular function redirect table.
The faithful implementations in `src/` run against the original in
`tests/differential/test_vendor_services.py` and `tests/differential/test_vendor_transport.py`.
The matching overrides in `enhanced/` run when the two DLLs are installed.

| Store address | Function | Enhanced behavior |
|---|---|---|
| `140787868` | `GdkLoadRuntime` | Resolves the GDK loader entry points from `t6_enhanced.dll`. The local exports return unavailable for Xbox service interfaces and discard GDK error reports. |
| `140788004` | `GdkSavePath` | Uses `%LOCALAPPDATA%\T6Enhanced`. Local GDK initialization creates the directory and publishes it before the filesystem starts. |
| `1407D0990` | `BdGetHostByName` | Returns zero addresses by default. Local Demonware development can resolve the auth and lobby hosts to loopback. |
| `1407D0DA0` | `BdSendTo` | Returns the original wrapper's network-failure result for Demonware datagrams. Ordinary engine UDP calls still go to Winsock. |
| `14049BD40` | `Sys_ReportUnhandledException` | Keeps the local fatal error and omits the call to Windows `ReportFault`. |
| `1404740C0` | `Sys_ProbeTimeServer` | Returns an unsuccessful time probe before opening a socket. |
| `140871040` | `BdConnectStream` | Returns the Demonware stream socket's network-failure result by default. Local Demonware development can open the loopback lobby stream. |
| `140491220` | `WinInetOpenConnection` | Returns no connection without reserving a request slot. |
| `140874870` | `BdBeginWinHttpRequest` | Completes the Demonware HTTP request with the standard connection-failure result. |
| `14088A0D0` | `XCurlAddRequest` | Maps the curl error and releases the incoming request. |
| `14088C600` | `WinHttpInitializeRequest` | Returns the standard connection-failure result before opening the request. |

The loader applies the redirects at the function entries above. The process keeps its original import table.
The differential tests model OS and library calls. XCurl's exceptional unwind paths still need verification.

The enhanced renderer uses standard Direct3D 11 and single-sample render targets if an adapter reports
NVIDIA but its NVAPI library or entry point is unavailable. A Wine smoke test with the startup assets from
R2 `bo2/DUMP/` displayed the multiplayer main menu and opened Options via keyboard with outbound connections
restricted. Omitting the English image archive left the menu labels invisible even when the background
artwork was present. Selecting Online still opens the game's online-service connection screen.

## Faithful service reconstruction

The Store client now has faithful source for GDK initialization, interface lookup and shutdown, XUser
add/result/duplicate/close and account-ID lookup, plus XThreading queue creation, dispatch, submission and close. The XUser and
XThreading wrappers preserve the shipped interface IDs, vtable slots, result propagation, and interface
release calls. That provides the contracts a local provider will need to implement.

Demonware's HTTP send and response completion paths preserve request handles, timeout and header setup,
body byte counts, status transitions, completion signals and reference release. The XCurl queue callback
preserves cancellation and propagation of transport failure to active requests. The first remote-task
reply path now covers byte-buffer reads, transaction IDs, reply attachment, pending queue removal and
reference ownership. The lobby service connect entry now covers the cached authentication record,
connection replacement and the ownership transfers into the connection constructor.
`tests/differential/test_service_lifecycle.py` compares those behaviors against the
pinned executable using modeled system calls and interface objects.

These functions are tracked as faithful reconstructions in `records/functions.tsv`. The standalone
service uses the Store client's native auth response parser and lobby connection.

## Standalone local Demonware service

Build the service with Go 1.24 or later from the repository root:

```powershell
Push-Location services/demonware
go build -o ../../build/t6mp_msstore/services/t6_demonware.exe .
Pop-Location
```

Run `t6_demonware.exe`, then run the Store game with the two enhanced DLLs installed:

```powershell
.\t6mp.exe --local-demonware
```

For Zombies, build with `python3 tools/build_runtime.py --build t6zm_msstore` and copy
`XINPUT1_4.dll` and `t6_enhanced_zm.dll` from `build/t6zm_msstore/runtime/` next to `t6zm.exe`.
Run `t6zm.exe --local-demonware` against the same service. In a directory containing both games,
install the shared loader alongside both enhanced DLLs.

The service listens on IPv4 loopback for Store auth on TCP 3080 and for the lobby on TCP 3074.
The auth request uses a synthetic local profile, and the game parses the returned tickets with its
original auth parser. The lobby listener validates the one-use server ticket and sends the
original protocol's connection-ID reply. It handles encrypted task replies, local time,
synthetic region data, and rich presence for one local profile. Its user-storage read and upload
tasks share an in-memory file store for that profile, which is cleared when the service exits.
The lobby replies to the client's keepalives and keeps authenticated connections open during map loads.
League team and member queries return empty results. Missing files return `BD_NO_FILE`. The service
acknowledges the startup console report and discards it. It returns `BD_SERVICE_NOT_AVAILABLE`
for the bandwidth test, whose untyped request header shares the reply queue with typed tasks.
The enhanced DLL configures the game's own multiplayer networking to use a loopback public
address in local Demonware mode. This uses the native address override and disables UPnP.

By default the service generates English publisher bundles for multiplayer and Zombies with
local playlist versions and synthetic league data. To supply a publisher bundle and files from disk, start the
service with `-publisher-dir <directory>`. Put the publisher
files directly in that directory under their requested names. The English Store client requested
`ffotd_tu17_mp_147.ff.00`, `online_tu17_mp_english.wad`, and `largeheatmap.raw` during startup.
Each file must fit in a single negotiated lobby frame.

In a loopback-only Wine run, the multiplayer client authenticated the lobby, accepted the server-time
and region replies, and requested and uploaded local profile files. The generated publisher
bundle set the playlist and league readiness flags. The client reported `0x1BFF`, which includes
the required `0x1BEE`, then opened the native **Online** menu and entered **Custom Games**.
**Start Match** loaded Aftermath in Team Deathmatch, reached class selection, and spawned the
player while the original authenticated lobby connection stayed open. The match clock started,
and keyboard input moved the player after the countdown.

In the same test environment, the Zombies client authenticated the loopback lobby, uploaded new
profile files, and reached the native **Online** menu. **Custom Games** opened the map, mode,
and difficulty selectors with a default for the publisher's newest-leaderboards setting. With
Wine and DXVK, **Start Match** loaded Green Run's Town Survival, spawned the player in Round 1,
and accepted movement and fire input while the original loopback lobby connection stayed open.
Green Run requires its mode and map-section fastfiles from the Store installation. Town Survival
uses `so_zsurvival_zm_transit.ff`, which contains the four player character scripts.

The Store build uses Demonware service `0x44`, task 1 for rich presence updates and task 2 for
queries. Task 1 serializes a zero user-ID field followed by a presence blob. Task 2 sends the
requested user IDs and attaches a result array. Xbox presence also has a separate
`XblPresenceSetPresenceAsync` path at `14082E7D0`.
