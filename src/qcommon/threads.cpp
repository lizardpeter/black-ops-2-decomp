// qcommon/threads.cpp (CoDMPServer_PC: threads.obj)
//
// The engine's threads. So far the per-thread values (the error handler's jump buffer, va's buffers, ...) and the quit
// event the render thread's message pump sets.
#define T6_SOURCE "qcommon/threads.cpp"

#include "qcommon/threads.h"
#include "t6_static.h"

#include <windows.h>

T6_STATIC HANDLE win32QuitEvent T6_AT("", "win32QuitEvent");
// Each thread's value table (Sys_InitThread points it at the thread's own; the name is the original's)
T6_STATIC T6_THREAD void **g_dwTlsIndex T6_AT("", "g_dwTlsIndex");

// 006DFD10  qcommon/threads.cpp:2258 (inlined in the Store build)
void Sys_SetValue(int valueIndex, void *data)
{
	g_dwTlsIndex[valueIndex] = data;
}

// 006DFD40  qcommon/threads.cpp:2266 (inlined in the Store build)
void *Sys_GetValue(int valueIndex)
{
	return g_dwTlsIndex[valueIndex];
}

// 006DFD60  qcommon/threads.cpp:2451 (inlined in the Store build)
void Sys_SetWin32QuitEvent()
{
	SetEvent(win32QuitEvent);
}

#ifdef T6_STORE
// ---- the render thread's side of the hand-off (Store only, inlined into RB_RenderThread; names made up) ----------
T6_STATIC void *volatile smpData T6_AT("", "smpData");          // the frame the main thread hands over
T6_STATIC HANDLE backendEvent[2] T6_AT("", "backendEvent");     // [1]: a frame is waiting
T6_STATIC HANDLE rendererRunningEvent T6_AT("", "rendererRunningEvent");
T6_STATIC HANDLE renderPausedEvent T6_AT("", "renderPausedEvent");

// Each pass of the render thread: renderPausedEvent goes down and up again around setting rendererRunningEvent.
void Sys_RendererRunning()
{
	ResetEvent(renderPausedEvent);
	SetEvent(rendererRunningEvent);
	SetEvent(renderPausedEvent);
}

// Whether the main thread has signalled a frame (backendEvent[1], auto-reset: the poll takes the signal).
bool Sys_RendererWorkPending()
{
	return WaitForSingleObject(backendEvent[1], 0) == WAIT_OBJECT_0;
}

// The frame handed over, taken (smpData left 0).
void *Sys_TakeRendererData()
{
	return InterlockedExchangePointer((void *volatile *)&smpData, 0);
}
#endif
