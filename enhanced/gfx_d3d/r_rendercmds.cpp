// enhanced/gfx_d3d/r_rendercmds.cpp: the hand-off of a frame to the render thread, timed (P0009).
//
// The faithful R_ToggleSmpFrameCmd (src/gfx_d3d/r_rendercmds.cpp) with its two waits timed for com_frameTiming: the
// wait for the render thread (with one frame on the CPU it is already done by then, and returns at once) and the
// Sleep(0) polls of the front end's jobs (B0028). Store build only.
#ifdef T6_STORE
#include "enhanced/frame_timing.h"

#include "gfx_d3d/r_rendercmds.h"
#include "qcommon/threads.h"
#include "universal/com_workercmds.h"
#include "universal/win_common.h"

#include <windows.h>

// src: Store 14071B880 R_ToggleSmpFrameCmd. Enhanced: its waits timed.
void R_ToggleSmpFrameCmd(unsigned int type)
{
	long long start = FrameTiming_Now();
	Sys_WaitRenderer();
	FrameTiming_Add(FRAME_TIMING_WAIT_RENDERER, start);
	start = FrameTiming_Now();
	while (!R_FinishedFrontendWorkerCmds())
		Sleep(0);
	FrameTiming_Add(FRAME_TIMING_WAIT_JOBS, start);
	R_UpdateSkinCacheUsage();
	Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
	GfxBackEndData *data = frontEndDataOut;
	R_ToggleSmpFrame();
	Sys_WakeRenderer(data);
	Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
}
#endif
