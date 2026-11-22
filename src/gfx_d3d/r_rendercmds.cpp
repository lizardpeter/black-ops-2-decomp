// gfx_d3d/r_rendercmds.cpp (CoDMPServer_PC: r_rendercmds.obj)
//
// The front end's render commands. So far the Store's hand-off of a finished frame to the render thread.
#define T6_SOURCE "gfx_d3d/r_rendercmds.cpp"

#include "gfx_d3d/r_rendercmds.h"
#include "qcommon/threads.h"
#include "t6_static.h"
#include "universal/com_workercmds.h"
#include "universal/win_common.h"

#include <windows.h>

#ifdef T6_STORE
extern float store_1427F1D6C[4];  // rg.skinnedVertexCacheUsed (the server's R_UpdateSkinCacheUsage); rg isn't mapped
extern void *store_142C3E760;     // name made up: cleared with the skin cache's usage (not in the server's build)

// Store: inlined into R_ToggleSmpFrameCmd 14071B880 and 14071BAC0 (the server's 00A2B4D0 also records a high-water
// mark; the Store's doesn't, and clears the pointer at 142C3E760). How full the frame's skinned vertex cache got.
void R_UpdateSkinCacheUsage()
{
	GfxBackEndData *data = frontEndDataOut;
	store_142C3E760 = 0;
	store_1427F1D6C[GfxBackEndData_viewInfoIndex(data)] =
		(float)GfxBackEndData_skinnedCacheVertsAllocated(data) / (float)GfxBackEndData_skinnedCacheVb(data)->total;
}

// Store 14071B880 (the server's 00A2F110, r_rendercmds.cpp:896). Hands the finished frame to the render thread: waits
// until the render thread is done with the last one, then until the front end's jobs are idle (Sleep(0) between
// polls), then under the device context's lock swaps the front end's frame and wakes the render thread with it.
void R_ToggleSmpFrameCmd(unsigned int type)
{
	Sys_WaitRenderer();
	while (!R_FinishedFrontendWorkerCmds())
		Sleep(0);
	R_UpdateSkinCacheUsage();
	Sys_EnterCriticalSection(CRITSECT_DXCONTEXT);
	GfxBackEndData *data = frontEndDataOut;
	R_ToggleSmpFrame();
	Sys_WakeRenderer(data);
	Sys_LeaveCriticalSection(CRITSECT_DXCONTEXT);
}
#endif
