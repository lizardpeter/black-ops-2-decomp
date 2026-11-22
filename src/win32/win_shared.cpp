// win32/win_shared.cpp (CoDMPServer_PC: win_shared.obj)
//
// Shared Windows code. So far the game's millisecond clock.
#define T6_SOURCE "win32/win_shared.cpp"

#include "t6_static.h"
#include "win32/win_shared.h"

#include <windows.h>

T6_GLOBAL int sys_timeBase;

// 0079D5D0  win32/win_shared.cpp:23 (inlined in the Store build)
// Milliseconds since the first call (timeGetTime, which wraps after 49.7 days).
int Sys_Milliseconds()
{
	T6_STATIC int initialized T6_AT("Sys_Milliseconds", "initialized");

	if (!initialized)
	{
		sys_timeBase = timeGetTime();
		initialized = 1;
	}
	return timeGetTime() - sys_timeBase;
}
