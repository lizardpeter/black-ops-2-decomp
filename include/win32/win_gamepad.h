// win32/win_gamepad.cpp
#pragma once

#include "qcommon/qcommon.h"

// One controller's state (0x7C bytes in both builds); only the fields reconstructed code reads.
struct GamePad
{
	bool enabled;
	unsigned char unknown1[0x7C - 1];
};
static_assert(sizeof(GamePad) == 0x7C, "GamePad");

bool GPad_IsActive(ControllerIndex_t portIndex);
bool GPad_InUse(LocalClientNum_t localClientNum);
