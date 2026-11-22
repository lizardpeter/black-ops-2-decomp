// win32/win_gamepad.cpp (CoDMPServer_PC: win_gamepad.obj)
//
// The gamepad. So far the checks the client's input code uses, and stopping the rumble.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_gamepad.cpp"
#define T6_SOURCE "win32/win_gamepad.cpp"

#include "t6_assert.h"
#include "t6_static.h"
#include "win32/win_gamepad.h"

#include <windows.h>

T6_STATIC GamePad s_gamePads[1] T6_AT("", "s_gamePads");

// 00797250  win32/win_gamepad.cpp:303 (inlined in the Store build)
// Both motors off; the controller is told when it is connected.
void GPad_StopRumbles(ControllerIndex_t portIndex)
{
	bcassert_at(306, portIndex, MAX_GPAD_COUNT);
	GamePad *pad = &s_gamePads[portIndex];
	pad->feedback.wLeftMotorSpeed = 0;
	pad->feedback.wRightMotorSpeed = 0;
	if (pad->enabled)
		XInputSetState(portIndex, (XINPUT_VIBRATION *)&pad->feedback);
}

// 007972D0  win32/win_gamepad.cpp:472 (inlined in the Store build). The bound is MAX_GPAD_COUNT, expanded in the
// assert's text.
bool GPad_IsActive(ControllerIndex_t portIndex)
{
	assert_at(475, (portIndex >= 0) && (portIndex < 1));
	return s_gamePads[portIndex].enabled;
}

// 007972B0  win32/win_gamepad.cpp:349 (inlined in the Store build)
bool GPad_InUse(LocalClientNum_t localClientNum)
{
	return Com_LocalClient_LastInput_Get(localClientNum) == LAST_INPUT_GAMEPAD;
}
