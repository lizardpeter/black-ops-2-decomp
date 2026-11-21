// game/gamerprofile.cpp: the player's profile settings.
#pragma once

#include "qcommon/qcommon.h"

enum profile_setting
{
	PROFILE_MOUSE_SENSITIVITY = 0x29,
};

int GamerProfile_GetInt(profile_setting setting, ControllerIndex_t controllerIndex);
float GamerProfile_GetFloat(profile_setting setting, ControllerIndex_t controllerIndex);
