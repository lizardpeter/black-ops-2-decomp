// client/: the client's input, connection and per-client state.
#pragma once

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

struct clientUIActive_t
{
	int flags;
	int keyCatchers;
	connstate_t connectionState;
	unsigned char unknownC[0x458 - 0xC];
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

struct clientActive_t
{
	unsigned char unknown0[0x44];
	bool usingAds;
	int timeoutcount;
	clSnapshot_t snap;
	unsigned char unknown2A98[0x2BCC - 0x2A98];
	bool stanceHeld;
	StanceState stance;
	StanceState stancePosition;
	int stanceTime;
	bool weapNextHeld;
	int weapNextTime;
	bool switchingToInventory;
	unsigned char unknown2BE8[0x2C8C - 0x2BE8];
	vec3_t viewangles;
	unsigned char unknown2C98[0x8A7B4 - 0x2C98];
	int lastFireTime;
	bool useHeld;
	int useTime;
	int useCount;
	// the rest follows (0x8A7E0 bytes in all)
};
static_assert(__builtin_offsetof(clientActive_t, snap) == 0x50, "clientActive_t.snap");
static_assert(__builtin_offsetof(clientActive_t, stance) == 0x2BD0, "clientActive_t.stance");
static_assert(__builtin_offsetof(clientActive_t, viewangles) == 0x2C8C, "clientActive_t.viewangles");
static_assert(__builtin_offsetof(clientActive_t, lastFireTime) == 0x8A7B4, "clientActive_t.lastFireTime");

// cl_main.cpp
extern unsigned int frame_msec;
clientActive_t *CL_GetLocalClientGlobals(LocalClientNum_t localClientNum);

// cl_input.cpp
bool IN_IsTalkKeyHeld();
