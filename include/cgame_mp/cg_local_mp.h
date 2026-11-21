// cgame_mp/cg_local_mp.h: the multiplayer cgame's per-client state, as far as reconstructed code reads it.
#pragma once

#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "bgame/bg_public.h"
#include "universal/q_shared.h"

// Only the fields reconstructed code uses, at each build's offsets: the Store revision's cg_t is laid out
// differently past predictedPlayerState (its offsets from the Store code named beside the static_asserts).
struct cg_t
{
	unsigned char unknown0[0x480A8];
	playerState_s predictedPlayerState;
#ifdef T6_STORE
	unsigned char unknown4AAB0[0x64B54 - 0x4AAB0];
	vec3_t damageKickAngles;
	unsigned char unknown64B60[0x6B554 - 0x64B60];
	Weapon weaponSelect;
#else
	unsigned char unknown4AAB0[0x64A74 - 0x4AAB0];
	vec3_t damageKickAngles;
	int previousRecoilTime;
	float previousRecoilRatio;
	unsigned char unknown64A88[0x69674 - 0x64A88];
	Weapon weaponSelect;
	int weaponSelectTime;
	bool weaponForcedSelect;
	unsigned char unknown69680[0x87F00 - 0x69680];
#endif
};
#ifdef T6_STORE
// t6mp_msstore: CG_IsSprinting 140192650 (predicted pm_flags at 0x480B4); CL_KeyMove 140192D50 (weaponSelect)
static_assert(__builtin_offsetof(cg_t, predictedPlayerState) == 0x480A8, "cg_t.predictedPlayerState");
static_assert(__builtin_offsetof(cg_t, weaponSelect) == 0x6B554, "cg_t.weaponSelect");
// CL_CreateCmd 140194E20 (CL_CopyDamageKickAngles inlined)
static_assert(__builtin_offsetof(cg_t, damageKickAngles) == 0x64B54, "cg_t.damageKickAngles");
#else
static_assert(sizeof(cg_t) == 0x87F00, "cg_t");
static_assert(__builtin_offsetof(cg_t, predictedPlayerState) == 0x480A8, "cg_t.predictedPlayerState");
static_assert(__builtin_offsetof(cg_t, damageKickAngles) == 0x64A74, "cg_t.damageKickAngles");
static_assert(__builtin_offsetof(cg_t, weaponSelect) == 0x69674, "cg_t.weaponSelect");
#endif

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

// cg_main_mp.cpp
const playerState_s *CG_GetPredictedPlayerState(LocalClientNum_t localClientNum);
