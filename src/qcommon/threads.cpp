// qcommon/threads.cpp (CoDMPServer_PC: threads.obj)
//
// The engine's threads. So far the per-thread values (the error handler's jump buffer, va's buffers, ...), which thread
// is which, the quit event the render thread's message pump sets, and the hand-off of a frame to the render thread.
#define T6_SOURCE "qcommon/threads.cpp"

#include "qcommon/qcommon.h"
#include "qcommon/threads.h"
#include "t6_static.h"
#include "universal/q_shared.h"

#include <windows.h>

T6_STATIC HANDLE win32QuitEvent T6_AT("", "win32QuitEvent");
// Each thread's value table (Sys_InitThread points it at the thread's own; the name is the original's)
T6_STATIC T6_THREAD void **g_dwTlsIndex T6_AT("", "g_dwTlsIndex");

// The calling thread's id, cached on first use (the name is the original's)
T6_GLOBAL T6_THREAD unsigned long g_currentThreadId;
// Each engine thread's id, by ThreadContext
T6_STATIC unsigned long threadId[THREAD_CONTEXT_COUNT] T6_AT("", "threadId");

// The calling thread's id (inlined into every caller in both builds)
static inline unsigned long Sys_GetCurrentThreadId()
{
	if (!g_currentThreadId)
		g_currentThreadId = GetCurrentThreadId();
	return g_currentThreadId;
}

// 006DFA60  qcommon/threads.cpp:1891 (inlined in the Store build)
bool Sys_IsServerThread()
{
	return Sys_GetCurrentThreadId() == threadId[THREAD_CONTEXT_SERVER];
}

// 006DFBD0  qcommon/threads.cpp:2185 (inlined in the Store build)
bool Sys_IsRenderThread()
{
	return Sys_GetCurrentThreadId() == threadId[THREAD_CONTEXT_BACKEND];
}

// 006DFC50  qcommon/threads.cpp:2208 (inlined in the Store build)
bool Sys_IsMainThread()
{
	return Sys_GetCurrentThreadId() == threadId[THREAD_CONTEXT_MAIN];
}

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
// ---- the hand-off of a frame to the render thread (Store only: the dedicated server has no render thread) ----------
T6_STATIC void *volatile smpData T6_AT("", "smpData");          // the frame the main thread hands over
T6_STATIC HANDLE backendEvent[2] T6_AT("", "backendEvent");     // [1]: a frame is waiting
T6_STATIC HANDLE rendererRunningEvent T6_AT("", "rendererRunningEvent");
T6_STATIC HANDLE renderPausedEvent T6_AT("", "renderPausedEvent");
T6_STATIC HANDLE renderCompletedEvent T6_AT("", "renderCompletedEvent");  // the render thread is done with the frame

// Store 1403C2C50 (the dedicated server compiles it to `return 0`, folded at 0055DF60, which R_ToggleSmpFrameCmd
// labels "wait renderer"; the name is made up). On the main thread of a client, waits until the render thread is done
// with the frame it was handed, in 1 ms timeouts (each returns as soon as the event is set).
int Sys_WaitRenderer()
{
	if (Com_IsDedicatedServer())
		return 0;
	if (Sys_IsMainThread())
	{
		while (WaitForSingleObject(renderCompletedEvent, 1))
		{
		}
	}
	return 0;
}

// 006E06C0  qcommon/threads.cpp:1734 (the server's; inlined in the Store build). Hands the render thread a frame.
void Sys_WakeRenderer(void *data)
{
	ResetEvent(renderCompletedEvent);
	smpData = data;
	SetEvent(backendEvent[1]);
	SetEvent(backendEvent[0]);
}

// ---- the render thread's side (inlined into RB_RenderThread; names made up) ----------------------------------------

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
