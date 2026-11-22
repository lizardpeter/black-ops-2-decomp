// universal/win_common.cpp (CoDMPServer_PC: win_common.obj)
//
// Shared Windows code. So far the engine's critical sections.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\universal\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_common.cpp"
#define T6_SOURCE "universal/win_common.cpp"

#include "t6_assert.h"
#include "t6_static.h"
#include "universal/win_common.h"

#include <windows.h>

T6_STATIC CRITICAL_SECTION s_criticalSection[CRITSECT_COUNT] T6_AT("", "s_criticalSection");

// 00792320  universal/win_common.cpp:489 (inlined in the Store build)
void Sys_EnterCriticalSection(CriticalSection critSect)
{
	bcassert_at(490, critSect, CRITSECT_COUNT);
	EnterCriticalSection(&s_criticalSection[critSect]);
}

// 007923C0  universal/win_common.cpp:501 (inlined in the Store build)
void Sys_LeaveCriticalSection(CriticalSection critSect)
{
	bcassert_at(502, critSect, CRITSECT_COUNT);
	LeaveCriticalSection(&s_criticalSection[critSect]);
}
