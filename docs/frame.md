# The frame: what one Store frame does, in order

Work in progress: the map the decoupled tick loop, the render-queue fix and the late latch are built on
(docs/roadmap.md). Addresses are the Store build's (`t6mp_msstore`); "server" means `CoDMPServer_PC`, whose
`Com_Frame` is the dedicated server's (the client half is compiled out), so the Store's client half has no
reference build and is read from its own disassembly. Nothing here is verified by a test yet.

## Com_Frame (1403A4010)

Found from `com_maxfps` (registered at 14039E43D, pointer 141F03820: its only reader is this function). The start
matches the server's `Com_Frame_Try_Block_Function` (006BF540, qcommon/common.cpp:6460) step for step:

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

## Still to map

- The client frame, `SCR_UpdateScreen` (server 00511320) and `CG_DrawActiveFrame` (server 004C7550) on the Store.
- Where the commands are built and sent (`CL_SendCmd`, server 004EC0A0) relative to the draw.
- `R_EndFrame` / `R_IssueRenderCommands` (server 00A2E4A0, 00A2F1E0), the render thread, `RB_SwapBuffers`
  (server 00A5A510) and the Present call; the swap chain's creation (`D3D11CreateDeviceAndSwapChain` and
  `CreateDXGIFactory1` are imported: 1408A8450, 1408A8460).
