// qcommon/threads.cpp (CoDMPServer_PC: threads.obj)
//
// The engine's threads. So far the quit event the render thread's message pump sets.
#define T6_SOURCE "qcommon/threads.cpp"

#include "qcommon/threads.h"
#include "t6_static.h"

#include <windows.h>

T6_STATIC HANDLE win32QuitEvent T6_AT("", "win32QuitEvent");

// 006DFD60  qcommon/threads.cpp:2451 (inlined in the Store build)
void Sys_SetWin32QuitEvent()
{
	SetEvent(win32QuitEvent);
}
