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
