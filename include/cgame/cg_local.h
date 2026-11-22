// cgame/cg_local.h: the cgame's shared state, as far as reconstructed code reads it.
#pragma once

#include "qcommon/qcommon.h"
#include "t6_assert.h"

struct AnimStateDef;

// A client entity (0x380 bytes on the server, 0x3D8 on the Store). Only the fields reconstructed code reads, with
// nextState's flattened to their offsets in the entity; the Store's nextState is 0x18 bytes further on.
struct centity_t
{
#ifdef T6_STORE
	unsigned char unknown0[0x1F4];
	unsigned char nextState[0x6A];  // entityState_s, read only through the fields below
	unsigned short actorAiType;  // nextState.lerp.u.actor.aiType
	unsigned char unknown260[0x266 - 0x260];
	unsigned short scriptMoverAiType;  // nextState.lerp.u.scriptMover.aiType (by the server's spacing; unverified)
	unsigned char unknown268[0x2CC - 0x268];
	short eType;  // nextState.eType
	unsigned char unknown2CE[0x3CC - 0x2CE];
	unsigned int packed_bits[2];  // bit 1: nextValid
	unsigned char unknown3D4[0x3D8 - 0x3D4];
#else
	unsigned char unknown0[0x1DC];
	unsigned char nextState[0x6A];  // entityState_s, read only through the fields below
	unsigned short actorAiType;     // nextState.lerp.u.actor.aiType
	unsigned char unknown248[0x24E - 0x248];
	unsigned short scriptMoverAiType;  // nextState.lerp.u.scriptMover.aiType
	unsigned char unknown250[0x2B4 - 0x250];
	short eType;  // nextState.eType
	unsigned char unknown2B6[0x378 - 0x2B6];
	unsigned int packed_bits[2];  // bit 1: nextValid
#endif
};
#ifdef T6_STORE
// t6mp_msstore: CL_CreateCmd 140194E20 (Demo_ClampViewAngleForLockedOnEntity and CG_GetEntity inlined)
static_assert(sizeof(centity_t) == 0x3D8, "centity_t (Store)");
static_assert(__builtin_offsetof(centity_t, eType) == 0x2CC, "centity_t.eType (Store)");
static_assert(__builtin_offsetof(centity_t, packed_bits) == 0x3CC, "centity_t.packed_bits (Store)");
#else
static_assert(sizeof(centity_t) == 0x380, "centity_t");
static_assert(__builtin_offsetof(centity_t, actorAiType) == 0x246, "centity_t.nextState.lerp.u.actor.aiType");
static_assert(__builtin_offsetof(centity_t, eType) == 0x2B4, "centity_t.nextState.eType");
#endif

enum
{
	ET_SCRIPTMOVER = 6,
	ET_HELICOPTER = 12,
	ET_ACTOR = 16,
	ET_ACTOR_CORPSE = 18,
};

#define MAX_LOCAL_CENTITIES ((1<<10) + 768)

extern centity_t *cg_entitiesArray[1];  // cg_main.cpp: per local client, allocated when the cgame starts

// AI types' theater lock-on limits (0x1C bytes)
struct AIDemoLockOnView
{
	int heightOffset;
	float pitchMin;
	float pitchMax;
};

struct AIDemoLockOnValues
{
	float highlightDistance;
	AIDemoLockOnView views[2];
};

// An AI type's script data (0x44 bytes on the server); only demoLockOn is read. The Store's has it at +0x48.
struct AITypeScript
{
#ifdef T6_STORE
	unsigned char unknown0[0x48];
#else
	int main;
	int precache;
	int spawner;
	unsigned char aiTypeIndex;
	unsigned char clientAiTypeIndex;
	int clientMain;
	int clientPrecache;
	void *footstepTable;
	void *footstepFXTable;
	AnimStateDef *animStateDef;
#endif
	AIDemoLockOnValues demoLockOn;
};
#ifndef T6_STORE
static_assert(__builtin_offsetof(AITypeScript, demoLockOn) == 0x24, "AITypeScript.demoLockOn");
#endif

// The cgame's level state; only aiTypes. The Store's base isn't located: its layout is anchored at aiTypes
// (140E59268, CL_CreateCmd 140194E20).
struct cg_level_locals_t
{
#ifndef T6_STORE
	unsigned char unknown0[0x3E1C];
#endif
	AITypeScript *aiTypes[255];
};

extern cg_level_locals_t cg_level;  // cg_main.cpp

// 00404450  cgame/cg_local.h:281 (inlined in the Store build). The dedicated server has no client entities: its
// build always calls Com_Error here (Com_IsDedicatedServer).
inline centity_t *CG_GetEntity(LocalClientNum_t localClientNum, int entityIndex)
{
	bcassert_in("c:\\t6\\code\\src\\cgame\\../cgame/cg_local.h", 282, localClientNum, MAX_LOCAL_CLIENTS);
	bcassert_in("c:\\t6\\code\\src\\cgame\\../cgame/cg_local.h", 283, entityIndex, MAX_LOCAL_CENTITIES);
	if (Com_IsDedicatedServer())
		Com_Error(ERR_DROP, "You can't call this on the dedicated server, there is no local client!\n");
	return &cg_entitiesArray[localClientNum][entityIndex];
}
