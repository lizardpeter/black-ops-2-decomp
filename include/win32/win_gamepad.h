// win32/win_gamepad.cpp
#pragma once

#include "qcommon/qcommon.h"

#define MAX_GPAD_COUNT 1

// XInput's rumble (XINPUT_VIBRATION)
struct GamePadFeedback
{
	unsigned short wLeftMotorSpeed;
	unsigned short wRightMotorSpeed;
};

// One controller's state (0x7C bytes in both builds); only the fields reconstructed code reads.
struct GamePad
{
	bool enabled;
	unsigned char unknown1[0x50 - 1];
	GamePadFeedback feedback;
	unsigned char unknown54[0x7C - 0x54];
};
static_assert(sizeof(GamePad) == 0x7C, "GamePad");
static_assert(__builtin_offsetof(GamePad, feedback) == 0x50, "GamePad");

bool GPad_IsActive(ControllerIndex_t portIndex);
void GPad_StopRumbles(ControllerIndex_t portIndex);
bool GPad_InUse(LocalClientNum_t localClientNum);
