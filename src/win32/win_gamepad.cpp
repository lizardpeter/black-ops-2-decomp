// win32/win_gamepad.cpp (CoDMPServer_PC: win_gamepad.obj)
//
// The gamepad. So far the check the client's mouse code uses.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_gamepad.cpp"
#define T6_SOURCE "win32/win_gamepad.cpp"

#include "win32/win_gamepad.h"

// 007972B0  win32/win_gamepad.cpp:349 (inlined in the Store build)
bool GPad_InUse(LocalClientNum_t localClientNum)
{
	return Com_LocalClient_LastInput_Get(localClientNum) == LAST_INPUT_GAMEPAD;
}
