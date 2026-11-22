# The enhanced build in the game

The enhanced Store build runs inside the real `t6mp.exe` (Store 1.1.8.0). The exe and every game file stay as they
are: two DLLs are added to the game folder, and deleting them puts the game back exactly as it was.

## What it changes so far

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
2. Copy both into the game folder, next to `t6mp.exe`.
3. Start multiplayer as usual. `t6_enhanced.log` appears next to the DLLs and should end with
   `enhanced build active`.

To remove it, delete `XINPUT1_4.dll` and `t6_enhanced.dll` (and the log).

## How it gets in

- **The loader** (`runtime/loader`) is `XINPUT1_4.dll`. `t6mp.exe` imports XInput by ordinal (2, 3, 4), and Windows
  looks in the game folder first. The loader passes every XInput call to the system's `xinput1_4.dll`, so
  controllers work as before.
- **Only the right exe is changed.** In `t6mp.exe` 1.1.8.0, recognised by its PE timestamp and image size, the
  loader loads `t6_enhanced.dll` and reads its `t6_redirects` table. In `t6sp.exe`, `t6zm.exe` or any other version,
  it only forwards XInput.
- **All or nothing.** The table lists each replaced exe function: its address, the replacement, and the 16 bytes
  the function started with when the DLL was built. The loader checks every entry first and patches nothing if any
  differs. Otherwise it writes a 14-byte jump at each entry, in memory only.
- **The DLL calls the exe's own code** (`tools/runtime_link.py`). A function the Store build identifies runs the
  exe's copy, not the reconstruction's. The DLL's own code is only the replacements and what the Store build
  inlines into the functions they replace, which is verified through those functions.

Checks before anything ships (the build fails otherwise):
- **Patch sites:** each replaced function is at least 14 bytes long (`.pdata`), and no branch in the exe lands in
  its first 14 bytes.
- **No traps:** nothing reachable from a replacement is something the Store build doesn't identify yet. That would
  crash in the game rather than fail a test. This follows calls, jumps, and references to functions such as the
  input thread's window procedure.
- **The package:** `tools/build_runtime.py` checks it as the loader will use it: the loader's XInput ordinals, its
  pinned timestamp and size, and each table entry's address, bytes and target.
- **Behaviour:** `tests/enhanced/test_mouse.py` covers each change against the original Store functions, and runs
  the exe's own patched entry points in the emulator.

What isn't covered: the loader and the input thread's Windows calls (thread, message window, raw input
registration) run only on Windows. Their logic around those calls (the window procedure, the counters) is tested
in the emulator.
