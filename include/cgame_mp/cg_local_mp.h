// cgame_mp/cg_local_mp.h: the multiplayer cgame's per-client state, as far as reconstructed code reads it.
#pragma once

#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "universal/q_shared.h"

struct cg_t
{
	unsigned char unknown0[0x64A74];
	vec3_t damageKickAngles;
	int previousRecoilTime;
	float previousRecoilRatio;
	unsigned char unknown64A88[0x69674 - 0x64A88];
	Weapon weaponSelect;
	int weaponSelectTime;
	bool weaponForcedSelect;
	unsigned char unknown69680[0x87F00 - 0x69680];
};
static_assert(sizeof(cg_t) == 0x87F00, "cg_t");
static_assert(__builtin_offsetof(cg_t, damageKickAngles) == 0x64A74, "cg_t.damageKickAngles");
static_assert(__builtin_offsetof(cg_t, weaponSelect) == 0x69674, "cg_t.weaponSelect");

extern cg_t *cgArray;  // cg_main_mp.cpp: allocated when the cgame starts

// 00404400  cgame_mp/cg_local_mp.h:2150
// Null until the cgame starts. One local client: the index is always 0.
inline cg_t *CG_GetLocalClientGlobals(LocalClientNum_t localClientNum)
{
	if (!cgArray)
		return 0;
	assertf_in("c:\\t6\\code\\src\\cgame\\../cgame_mp/cg_local_mp.h", 2168, (localClientNum == 0),
	           "(localClientNum) = %i", localClientNum);
	return cgArray;
}
