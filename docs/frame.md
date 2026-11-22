# The frame: what one Store frame does, in order

Work in progress: the map the decoupled tick loop, the render-queue fix and the late latch are built on
(docs/roadmap.md). Addresses are the Store build's (`t6mp_msstore`); "server" means `CoDMPServer_PC`, whose
`Com_Frame` is the dedicated server's (the client half is compiled out), so the Store's client half has no
reference build and is read from its own disassembly. Nothing here is verified by a test yet.

## Com_Frame_Try_Block_Function (1403A4010)

Found from `com_maxfps` (registered at 14039E43D, pointer 141F03820: its only reader is this function); the
matcher names its caller `Com_Frame` (1403A5960, method `caller`). The start matches the server's `Com_Frame_Try_Block_Function` (006BF540, qcommon/common.cpp:6460) step for step:

1. `minMsec` = 1000 / `com_maxfps`, at least 1 (no cap when `com_maxfps` is 0). A second cap, `[140E53B70]+0x1C54`,
   applies when it is positive and lower (not yet identified; the server has `com_timescaleValue < 1` here instead).
2. **The frame wait** ("progress time spin"): `Com_EventLoop` (14039BC20), then `Sys_Milliseconds`; while less than
   `minMsec` has passed since `com_frameTime` (141F04894), `Sleep(1)` and loop, at most 50 times. So the cap waits
   in 1 ms sleeps (with `timeBeginPeriod(1)`), and **keys are taken before the wait, not after it**.
3. `com_frameTime` = now; the frame's `msec` = the time since the last frame, 1 to 5000 (100 when a dvar at
   141F048F0 is set and not dedicated; not yet identified).
4. **Frame-time smoothing:** for frames up to 40 ms, `msec` is replaced by an average over the last three frames
   (`com_lastFrameTime[3]` at 143F81DE0, the float `com_lastFrameTime_base[3]` at 143F81E30, index
   `com_lastFrameIndex % 3` at 143F81DEC), as in the server. Longer frames reset the history. This is what the
   commands' and the client's frame times are made from.

Later in the same function (client half, Store only; to be read in full):

- the client's frame times: `cls.frametime` (142CB7DD0) = `msec`, `cls.frametime_base` (+4) = the smoothed seconds,
  `cls.realtime` (+8) += `msec`, `cls.realFrametime` (+0xC);
- `IN_Frame` (14049A0A0) when `cls.inputUpdatedPrevFrame` is clear (CL_Input sets it when it runs IN_Frame itself),
  and then **a second `Com_EventLoop`** (1403A470B), so keys pressed during the frame wait reach this frame there;
- the client frame and the screen update between 1403A4722 and 1403A47E7 (not yet identified);
- at the end, a second wait: `Sleep(1)` until `Sys_Milliseconds()` reaches the value at 1427F1B58, which is set from
  `com_lastFrameTime[index]` (1403A467C). It compares a time with a frame length, so it looks like it only waits in
  the first moments after start; to be confirmed.

## Frame-rate limits

- `com_maxfps` (141F03820, registered at 14039E43D): an int dvar, 0 to 1000; 0 is no cap.
- `[140E53B70]+0x1C54`, the second cap in step 1, is `sv_clientFpsLimit` from the server's info string (stored at
  140152095 with the other server settings): a server can lower its clients' frame rate.
- Even with no cap, `minMsec` is 1 and every frame is a whole number of milliseconds of at least 1: at most 1000
  frames a second.
- Presentation (140735BC0, the Store's `RB_SwapBuffers`): `IDXGISwapChain::Present(sync, 0)`, sync 1 when `r_vsync`
  (1427E70C8) or the dvar at 1427E6FB8 (registered next to `r_loadingScreen`) is set, else 0. No
  `DXGI_PRESENT_ALLOW_TEARING`. The device and swap chain come from `D3D11CreateDeviceAndSwapChain` (140720192), or
  NVAPI on NVIDIA hardware (vendor 0x10DE, 14072002D); the swap chain description comes from the caller (not yet read).

## The call chain to the draw and the present

From the matcher's candidates (analysis/t6mp_msstore/match/functions.tsv; not yet in records/names.tsv) and the
Store's call graph:

- `Com_Frame` 1403A5960 → `Com_Frame_Try_Block_Function` 1403A4010 → **1401BE780**, which the server's callers of
  `SCR_UpdateScreen` (00511320) point to: it is also called from `CL_DownloadsComplete` (1401AA200), as on the
  server. → **1401BDA90** (`SCR_UpdateFrame`'s place) → 1401BC300 → `CG_DrawActiveFrame` 14016F770.
- **Commands are built during the draw:** `CG_DrawActiveFrame` calls `CL_Input` (1401974A0, verified), which runs
  `IN_Frame` (the mouse) and `CL_CreateNewCommands`. So the mouse is read after the frame wait and the second
  `Com_EventLoop`, at draw time, as in BO3. (On the server `CL_SendCmd` is called from `CG_PredictPlayerState`; the
  Store's 140124580 is that function.)
- **Present:** `RB_SwapBuffers` 140735BC0 is reached from 14071BAC0, which `SCR_UpdateFrame`'s place 1401BDA90 calls.
- **Two ways to the present.** The game starts a render thread at 1403A1715: entry 140736CD0, through the thread
  helper 1403C2BD0 (its handles at 14406FC48 and 144070008, events created just before). On that thread 140736950
  runs the back end and calls `RB_SwapBuffers`: the server's `RB_RenderThread` / `RB_ExecuteRenderCommandsLoop`
  shape. 14071BAC0 (called from the frame update; the server's `R_EndFrame` / `R_IssueRenderCommands` place) can
  also run the back end in line when the flag at 1427E7338 is set: `RB_BeginFrame` (140735910), `RB_Draw3D`
  (140736120), 1407366F0, then `RB_SwapBuffers` under the critical section at 1441FB970. The flag at 1427E7338 is set only
  while the render thread itself runs the screen update (1401BE780) in a loop, as long as 1427E7340 is set
  (1407370A0): the loading-screen mode. **A normal frame takes the render thread.**
- **The render thread's loop** (140736CD0): an error-recovery point (140870ACC, setjmp), then it waits for work on
  the events at 14406FC58 and 144070008. A frame arrives as a pointer swapped out of 14406FAF0 (`XCHG`); under the
  critical section 1441FB970 it uploads the frame's dynamic vertex data (Map/Unmap on the device context 1427F1D90,
  looping while 142155DFC is set) and finishes three command lists (140736BA0 at +0x122C10, +0x122C00, +0x122C50 of
  the frame), then runs the back end and presents (140736950). 
- **The hand-off** (14071B880, the server's `R_IssueRenderCommands` place, called from the end-of-frame 14071BAC0):
  1. 1403C2C50 (the server's `R_SyncRenderThread` / `Sys_WaitRenderer` place): on the main thread (thread id
     14406FBF0), `WaitForSingleObject(14406FBE0, 1)` in a loop until the render thread has signalled that it is done
     with the previous frame. 1 ms waits: the main thread wakes late by up to about 1 ms.
  2. A spin: while any of 15 job queues (140D3A5A8 ... 143043BC8, tested by 14078C880) still has work,
     `Sleep(0)` and start again from 1. That is a busy wait on the main thread.
  3. Under the critical section 1441FB970: finish the frame's data (14071BFA0), `ResetEvent(14406FBE0)`, publish the
     frame (`14406FAF0` = the frame's back-end data, 1427E7320), `SetEvent(14406FC58)` (work for the render thread),
     `SetEvent(14406FC50)`.
- **The render thread signals done before it presents** (140736950): `SetEvent(14406FBE0)` and `SetEvent(14406FC50)`
  at 140736AC5, then `RB_SwapBuffers` (`Present`) at 140736AEB. So the main thread builds frame N+1 while the render
  thread presents frame N, and the render thread blocks in `Present` when DXGI's queue is full (by default up to 3
  frames). **The frame queue is therefore one frame on the render thread plus DXGI's queue:** the main thread can run
  up to about four frames ahead of the screen when the GPU is the bottleneck. This is what the latency work changes:
  a frame-latency waitable object on a flip-model swap chain with maximum latency 1, waited on by the main thread
  before it reads input and builds the frame (Ghosts P0004 waits at the same point, its `R_SyncGpu`).

## The swap chain

Built by 14071E360 for the device creation 14071FEA0 (called from the renderer's init 1407203B0):

| `DXGI_SWAP_CHAIN_DESC` field | Value |
|---|---|
| `BufferDesc.Width`, `Height` | the window's size (from the caller's parameters) |
| `BufferDesc.RefreshRate` | 0/0 (no refresh rate asked for) |
| `BufferDesc.Format` | 0x1C, `DXGI_FORMAT_R8G8B8A8_UNORM` |
| `SampleDesc` | 1, 0 (multisampling is done on its own targets: `r_aaSamples`-style value kept at 14282E538) |
| `BufferUsage` | 0x20, `DXGI_USAGE_RENDER_TARGET_OUTPUT` |
| `BufferCount` | 2 |
| `Windowed` | TRUE: exclusive fullscreen is entered afterwards with `SetFullscreenState` (vtable +0x50, 140720EB0 and others) |
| `SwapEffect` | 0, `DXGI_SWAP_EFFECT_DISCARD`: the old blt model |
| `Flags` | 2, `DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH` |

`r_fullscreen` 2 (1407204D3) is the borderless window: 1407206E0 restyles the window (`SetWindowLongA`,
`AdjustWindowRectEx`, `SetWindowPos`). With the blt model, a borderless window goes through the desktop compositor:
an extra frame of latency and no tearing, so it never shows more than one frame a refresh. Exclusive fullscreen avoids
the compositor but has slow alt-tab and mode switches. The enhanced build's flip model
(`DXGI_SWAP_EFFECT_FLIP_DISCARD`, `ALLOW_TEARING`, `FRAME_LATENCY_WAITABLE_OBJECT`, at least 2 buffers) makes the
borderless window as fast as exclusive fullscreen; Windows' independent flip takes over when the window covers the
screen.

## What the enhanced build changes, and what has to be rebuilt first

The order the work goes in (each step rebuilt and verified faithful first, then changed in `enhanced/`):

1. **The swap chain description** (14071E360, no server counterpart: the server's renderer has no device code).
   Enhanced: `DXGI_SWAP_EFFECT_FLIP_DISCARD`, `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` when supported,
   `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`. One description serves both device paths (D3D11 and NVAPI).
2. **Device creation** (14071FEA0): after creation, `SetMaximumFrameLatency(1)` and the swap chain's waitable object
   (IDXGISwapChain2); falling back to the original swap chain where the system refuses the flip model, as Ghosts
   P0003 does.
3. **The resize and fullscreen paths** (`ResizeBuffers`, vtable +0x68, at 14072F5A7, 1407EDEBB, 1407F4B9B;
   `SetFullscreenState` at 140720EB0 and others): the flags must match the swap chain's.
4. **`RB_SwapBuffers`** (140735BC0): `DXGI_PRESENT_ALLOW_TEARING` when v-sync is off in a window; the flip model
   unbinds the back buffer at `Present`, so it is bound again after it.
5. **`Com_Frame_Try_Block_Function`** (1403A4010): the central rebuild. The frame-latency wait goes right before the
   second `Com_EventLoop` (1403A470B), so keys (that event loop) and the mouse (`CL_Input`, at draw time) are both
   taken after it, by the original's own code in its own order (BO3's P0020 arrangement; no mid-frame filter needed).
   The same rebuild gives the microsecond frame clock, the waitable-timer cap, and later the 5 ms tick loop.
6. **The hand-off wait** (1403C2C50, 1-ms `WaitForSingleObject` steps) and the job-queue spin with `Sleep(0)` in
   14071B880: wait without polling.

## Still to map

- The client frame, `SCR_UpdateScreen` (server 00511320) and `CG_DrawActiveFrame` (server 004C7550) on the Store.
- Where the commands are built and sent (`CL_SendCmd`, server 004EC0A0) relative to the draw.
- `R_EndFrame` / `R_IssueRenderCommands` (server 00A2E4A0, 00A2F1E0), the render thread, `RB_SwapBuffers`
  (server 00A5A510) and the Present call; the swap chain's creation (`D3D11CreateDeviceAndSwapChain` and
  `CreateDXGIFactory1` are imported: 1408A8450, 1408A8460).
