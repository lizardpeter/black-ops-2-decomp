// client/: the client's input, connection and per-client state.
#pragma once

#include "gfx_d3d/r_vidconfig.h"

#include "bgame/bg_public.h"
#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "universal/q_shared.h"

struct dvar_t;

struct kbutton_t
{
	int down[2];          // the keys holding it down
	unsigned int downtime;
	unsigned int msec;    // time held since the last CL_KeyState
	bool active;          // down now
	bool wasPressed;      // went down since the last command
	float val;            // analog value of the key holding it
};

enum
{
	KB_LEFT, KB_RIGHT, KB_FORWARD, KB_BACK, KB_LOOKUP, KB_LOOKDOWN, KB_MOVELEFT, KB_MOVERIGHT, KB_STRAFE, KB_SPEED,
	KB_UP, KB_DOWN, KB_ANYUP, KB_MLOOK, KB_SPECTATE, KB_SPECNEXT, KB_SPECPREV, KB_ATTACK, KB_BREATH, KB_FRAG,
	KB_OFFHANDSECONDARY, KB_THROWBACK, KB_MELEE, KB_ACTIVATE, KB_RELOAD, KB_USE_RELOAD, KB_LEANLEFT, KB_LEANRIGHT,
	KB_PRONE, KB_CROUCH, KB_THROW, KB_STANCE, KB_SPRINT, KB_NIGHTVISION, KB_GAS, KB_REVERSE, KB_HANDBRAKE,
	KB_SWITCHSEAT, KB_VEHICLEATTACK, KB_VEHICLEATTACKSECOND, KB_VEHICLEMOVEUP, KB_VEHICLEMOVEDOWN, KB_VEHICLEBOOST,
	KB_JAM, KB_TALK, NUM_BUTTONS
};

enum StanceState
{
	CL_STANCE_STAND = 0,
	CL_STANCE_CROUCH = 1,
	CL_STANCE_PRONE = 2,
	CL_STANCE_DIVE_TO_PRONE = 3,
};

enum connstate_t
{
	CA_DISCONNECTED, CA_CINEMATIC, CA_UICINEMATIC, CA_LOGO, CA_CONNECTING, CA_CHALLENGING, CA_CONNECTED,
	CA_SENDINGSTATS, CA_LOADING, CA_PRIMED, CA_ACTIVE
};

enum clientMigState_t
{
	CMSTATE_INACTIVE = 0,
	CMSTATE_OLDHOSTLEAVING = 1,
	CMSTATE_LIMBO = 2,
	CMSTATE_NEWHOSTCONNECT = 3,
	CMSTATE_COUNT = 4,
};

struct clientUIActive_t
{
	int flags;
	int keyCatchers;
	connstate_t connectionState;
	int unknownC;
	clientMigState_t migrationState;
	unsigned char unknown14[0x458 - 0x14];
};
static_assert(sizeof(clientUIActive_t) == 0x458, "clientUIActive_t");

extern clientUIActive_t clientUIActives[1];  // cl_main.cpp

// client/client.h's per-local-client accessors (the asserts name the header as client_mp's sources include it). One
// local client: the index is always 0.

// 00464890  client/client.h:639
inline clientUIActive_t *CL_GetLocalClientUIGlobals(LocalClientNum_t localClientNum)
{
	assertf_in("c:\\t6\\code\\src\\client_mp\\../client/client.h", 644, (localClientNum == 0),
	           "(localClientNum) = %i", localClientNum);
	return &clientUIActives[0];
}

// 0041D9A0  client/client.h:696
inline connstate_t CL_GetLocalClientConnectionState(LocalClientNum_t localClientNum)
{
	assertf_in("c:\\t6\\code\\src\\client_mp\\../client/client.h", 701, (localClientNum == 0),
	           "(localClientNum) = %i", localClientNum);
	return clientUIActives[0].connectionState;
}

// 00426AD0  client/client.h:753
inline clientMigState_t CL_GetLocalClientMigrationState(LocalClientNum_t localClientNum)
{
	assertf_in("c:\\t6\\code\\src\\client_mp\\../client/client.h", 758, (localClientNum == 0),
	           "(localClientNum) = %i", localClientNum);
	return clientUIActives[0].migrationState;
}

struct alignas(8) clSnapshot_t  // playerState_s holds 8-byte members
{
	int valid;
	int snapFlags;
	int serverTime;
	int physicsTime;
	int messageNum;
	int deltaNum;
	int ping;
	int cmdNum;
	playerState_s ps;
	unsigned char unknown2A28[0x2A48 - 0x2A28];
};
static_assert(sizeof(clSnapshot_t) == 0x2A48, "clSnapshot_t");

struct MatchState;
struct entityState_s;
struct clientState_s;
struct actorState_s;
struct outPacket_t;

// The server PDB's declaration. Pointer fields keep their types, so the Store build's x64 layout follows (they
// widen to 8 bytes: everything from usingAds moves 0x18 further in), and its smaller usercmd_s shrinks the ring.
struct clientActive_t
{
	MatchState *parseMatchStatesBuf;
	int numParseMatchStates;
	int maxParseMatchStates;
	entityState_s *parseEntitiesBuf;
	int numParseEntities;
	int maxParseEntities;
	clientState_s *parseClientsBuf;
	int numParseClients;
	int maxParseClients;
	actorState_s *parseActorsBuf;
	int numParseActors;
	int maxParseActors;
	outPacket_t *outPackets;
	clSnapshot_t *snapshots;
	int packetBackupCount;
	int packetBackupMask;
	int gameStateMarker;
	bool usingAds;
	int timeoutcount;
	clSnapshot_t snap;
	unsigned char unknown2A98[0x2BCC - 0x2A98];  // snapServerTime .. parseActorsIndex
	bool stanceHeld;
	StanceState stance;
	StanceState stancePosition;
	int stanceTime;
	bool weapNextHeld;
	int weapNextTime;
	bool switchingToInventory;
	Weapon cgameUserCmdWeapon;
	Weapon cgameUserCmdOffHandWeapon;
	Weapon cgameUserCmdLastWeaponForAlt;
	float cgameFOVSensitivityScale;
	float cgameMaxPitchSpeed;
	float cgameMaxYawSpeed;
	unsigned char unknown2C00[0x2C8C - 0x2C00];  // cgameKickAngles .. cgamePredictedDataServerTime
	vec3_t viewangles;
	int skelTimeStamp;
	volatile int skelMemPos;
	char skelMemory[262144];
	char *skelMemoryStart;
	bool allowedAllocSkel;
	usercmd_s cmds[128];  // a ring: cmdNumber & 0x7F
	int cmdNumber;
	unsigned char unknown44EAC[0x8A7B4 - 0x44EAC];  // clientArchive .. translationVersion
	int lastFireTime;
	bool useHeld;
	int useTime;
	int useCount;
	int wasInVehicle;
	int serverFrameMS;
	int serverKbps;
	int serverKbpsAvg[2];
	int serverKbpsCount[2];
};
#ifdef T6_STORE
// t6mp_msstore, from its code: CL_KeyMove 140192D50, CL_MouseMove 1401930F0, CL_CreateNewCommands 1401954D0
static_assert(__builtin_offsetof(clientActive_t, usingAds) == 0x5C, "clientActive_t.usingAds");
static_assert(__builtin_offsetof(clientActive_t, snap) == 0x68, "clientActive_t.snap");
static_assert(__builtin_offsetof(clientActive_t, stanceHeld) == 0x2BE4, "clientActive_t.stanceHeld");
static_assert(__builtin_offsetof(clientActive_t, switchingToInventory) == 0x2BFC, "clientActive_t.switchingToInventory");
static_assert(__builtin_offsetof(clientActive_t, cgameFOVSensitivityScale) == 0x2C0C, "clientActive_t.cgameFOVSensitivityScale");
static_assert(__builtin_offsetof(clientActive_t, cgameMaxYawSpeed) == 0x2C14, "clientActive_t.cgameMaxYawSpeed");
static_assert(__builtin_offsetof(clientActive_t, viewangles) == 0x2CA4, "clientActive_t.viewangles");
static_assert(__builtin_offsetof(clientActive_t, cmds) == 0x42CC4, "clientActive_t.cmds");
static_assert(__builtin_offsetof(clientActive_t, cmdNumber) == 0x44AC4, "clientActive_t.cmdNumber");
static_assert(__builtin_offsetof(clientActive_t, useTime) == 0x8A3D8, "clientActive_t.useTime");
static_assert(__builtin_offsetof(clientActive_t, wasInVehicle) == 0x8A3E0, "clientActive_t.wasInVehicle");
#else
static_assert(sizeof(clientActive_t) == 0x8A7E0, "clientActive_t");
static_assert(__builtin_offsetof(clientActive_t, cgameMaxYawSpeed) == 0x2BFC, "clientActive_t.cgameMaxYawSpeed");
static_assert(__builtin_offsetof(clientActive_t, wasInVehicle) == 0x8A7C4, "clientActive_t.wasInVehicle");
static_assert(__builtin_offsetof(clientActive_t, cmdNumber) == 0x44EA8, "clientActive_t.cmdNumber");
static_assert(__builtin_offsetof(clientActive_t, snap) == 0x50, "clientActive_t.snap");
static_assert(__builtin_offsetof(clientActive_t, stance) == 0x2BD0, "clientActive_t.stance");
static_assert(__builtin_offsetof(clientActive_t, viewangles) == 0x2C8C, "clientActive_t.viewangles");
static_assert(__builtin_offsetof(clientActive_t, lastFireTime) == 0x8A7B4, "clientActive_t.lastFireTime");
#endif

struct clientMouse_t
{
	int mouseDx[2];
	int mouseDy[2];
	int mouseIndex;
};

// usercmd_s.button_bits (bitarray<64>, bit 0 the first word's top bit). Names are descriptive, from what cl_input.cpp
// does with them.
enum
{
	BUTTON_SPRINT = 1,
	BUTTON_PRONE = 8,
	BUTTON_CROUCH = 9,
	BUTTON_ADS = 11,
	BUTTON_STANCE_KEY = 12,             // a hold-to-crouch or hold-to-prone key is down
	BUTTON_DIVE_TO_PRONE = 44,
	BUTTON_USE_DOUBLE_TAP = 50,
	BUTTON_AIRBURST_MARK = 51,          // usercmd_s.airburstMarkDistance holds a distance
	BUTTON_SWITCH_TO_INVENTORY = 53,
};

// The Store revision has one int fewer before frametime (its cls is at 142CB7C80: hunkUsersStarted set to 1 at
// 1401A9E49, servername at +4; CL_CreateCmd 140194E20 reads frametime at +0x150, CL_Input 1401974A0 writes
// inputUpdatedPrevFrame at +0x160). Its size isn't known.
struct clientStatic_t
{
#ifdef T6_STORE
	unsigned char unknown0[0x150];
#else
	unsigned char unknown0[0x154];
#endif
	int frametime;  // milliseconds this frame
	float frametime_base;
	int realtime;
	int realFrametime;
	bool inputUpdatedPrevFrame;
#ifdef T6_STORE
	unsigned char unknown161[0x89D0 - 0x161];
#else
	unsigned char unknown165[0x89B4 - 0x165];
#endif
	vidConfig_t vidConfig;  // the Store's ScrPlace_HiResApplyRect (in ScrPlace_ApplyRect 1401F65E0) reads +8 and +0xC
#ifndef T6_STORE
	unsigned char unknown89F4[0x5EF88 - 0x89F4];
#endif
};
#ifdef T6_STORE
static_assert(__builtin_offsetof(clientStatic_t, inputUpdatedPrevFrame) == 0x160, "clientStatic_t (Store)");
static_assert(__builtin_offsetof(clientStatic_t, vidConfig) == 0x89D0, "clientStatic_t (Store)");
#else
static_assert(sizeof(clientStatic_t) == 0x5EF88, "clientStatic_t");
static_assert(__builtin_offsetof(clientStatic_t, inputUpdatedPrevFrame) == 0x164, "clientStatic_t");
static_assert(__builtin_offsetof(clientStatic_t, vidConfig) == 0x89B4, "clientStatic_t");
#endif

extern clientStatic_t cls;  // cl_main.cpp

// cl_main.cpp
extern unsigned int frame_msec;
extern int cl_maxLocalClients;
extern clientActive_t *clients;  // allocated for cl_maxLocalClients

// 0042E740  client/client.h:665 (the Store build inlines it everywhere). Returns the one clientActive_t, or null for
// an index past cl_maxLocalClients.
inline clientActive_t *CL_GetLocalClientGlobals(LocalClientNum_t localClientNum)
{
	if (localClientNum >= cl_maxLocalClients)
		return 0;
	assert_in("c:\\t6\\code\\src\\client_mp\\../client/client.h", 669, clients);
	assertf_in("c:\\t6\\code\\src\\client_mp\\../client/client.h", 676, (localClientNum == 0),
	           "(localClientNum) = %i", localClientNum);
	return clients;
}

// cl_input.cpp
bool IN_IsTalkKeyHeld();
