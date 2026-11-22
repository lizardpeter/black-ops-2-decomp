// live/live_win.cpp (CoDMPServer_PC: live_win.obj)
//
// Live on Windows. So far whether the system UI (the platform's overlay) is up.
#define T6_SOURCE "live/live_win.cpp"

#include "t6_static.h"
#include "live/live.h"

#ifdef T6_STORE
T6_STATIC LiveSystemUi s_systemUi[1] T6_AT("", "s_systemUi");

// live/live_win.cpp (inlined in the Store build): the Store revision asks per controller
bool Live_IsSystemUiActive(ControllerIndex_t controllerIndex)
{
	return s_systemUi[controllerIndex].active;
}
#else
T6_STATIC bool s_systemUiActive T6_AT("", "s_systemUiActive");

// 008EEE10  live/live_win.cpp:1844
bool Live_IsSystemUiActive()
{
	return s_systemUiActive;
}
#endif
