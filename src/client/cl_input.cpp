// client/cl_input.cpp (CoDMPServer_PC: cl_input.obj)
//
// Keyboard and mouse into user commands: the button state (kbutton_t) the bound commands press and release, the
// stance and weapon buttons, and building each frame's usercmd_s.
//
// The server's link-time code generation gave most of these functions custom calling conventions (registers listed
// per function in records/conventions.tsv); the reconstruction uses the standard one, and the tests and the hybrid
// link adapt.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\client\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "cl_input.cpp"
#define T6_SOURCE "client/cl_input.cpp"

#include "client/client.h"

#include <stdlib.h>

#include "bgame/bg_public.h"
#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_compiler.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_shared.h"

// cgame
const playerState_s *CG_GetPredictedPlayerState(LocalClientNum_t localClientNum);
void CG_CycleWeapon(LocalClientNum_t localClientNum, int cycleForward);

extern const dvar_t *cl_dblTapMaxHoldTime;   // universal/dvar.cpp
extern const dvar_t *cl_dblTapMaxDelayTime;  // universal/dvar.cpp

T6_GLOBAL const dvar_t *cl_leanCountMin;
T6_GLOBAL int g_leaned;
T6_GLOBAL bool gAttackEdgeDetected[1];

T6_STATIC kbutton_t playersKb[1][NUM_BUTTONS] T6_AT("", "playersKb");

// 004EA830  client/cl_input.cpp:83
void CL_LeanCount_Reset()
{
	g_leaned = 0;
}

// 004EA840  client/cl_input.cpp:88
void CL_LeanCount_Increment()
{
	if (Dvar_GetInt(cl_leanCountMin))
		++g_leaned;
}

// 004EA860  client/cl_input.cpp:99
bool CL_LeanCount_ShouldIncrementStat()
{
	return Dvar_GetInt(cl_leanCountMin) && g_leaned >= Dvar_GetInt(cl_leanCountMin);
}

// 004EA8A0  client/cl_input.cpp:140
bool CG_IsSprinting(LocalClientNum_t localClientNum)
{
	const playerState_s *ps = CG_GetPredictedPlayerState(localClientNum);
	return ps->viewHeightCurrent >= (float)Dvar_GetInt(player_standingViewHeight) && (ps->pm_flags & PMF_SPRINTING);
}

// 004EA8E0  client/cl_input.cpp:164
// A key bound to the button went down. Two keys can hold a button.
void IN_KeyDown(kbutton_t *b, int k, unsigned int time, float value)
{
	b->val = value;
	if (k == b->down[0] || k == b->down[1])
		return;  // a repeating key
	if (!b->down[0])
	{
		b->down[0] = k;
	}
	else if (!b->down[1])
	{
		b->down[1] = k;
	}
	else
	{
		Com_Printf(25, "Three keys down for a button!\n");
		return;
	}
	if (b->active)
		return;
	b->downtime = time;
	b->active = true;
	b->wasPressed = true;
}

// 004EA930  client/cl_input.cpp:205
// A key bound to the button went up; the button's held time grows by how long it was down (half a frame when the
// key gives no time).
void IN_KeyUp(kbutton_t *b, int k, unsigned int time)
{
	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;  // a key that wasn't holding it
	if (b->down[0] || b->down[1])
		return;  // still held by the other key
	b->active = false;
	if (time)
		b->msec += time - b->downtime;
	else
		b->msec += frame_msec / 2;
	b->val = 0.0f;
	b->active = false;
}

// 004EA990  client/cl_input.cpp:241
// The +button command: argument 1 the key (none: typed at the console, held until released), 2 the time, 3 the
// analog value.
void IN_KeyDownCheat(kbutton_t *b)
{
	const char *c = Cmd_Argv(1);
	int k = c[0] ? atoi(c) : -1;
	c = Cmd_Argv(3);
	b->val = c[0] ? (float)atof(c) : 1.0f;
	if (k == b->down[0] || k == b->down[1])
		return;
	if (!b->down[0])
	{
		b->down[0] = k;
	}
	else if (!b->down[1])
	{
		b->down[1] = k;
	}
	else
	{
		Com_Printf(25, "Three keys down for a button!\n");
		return;
	}
	if (b->active)
		return;
	b->downtime = atoi(Cmd_Argv(2));
	b->active = true;
	b->wasPressed = true;
}

// 004EAA30  client/cl_input.cpp:298
// The -button command. With no key, releases the button whatever holds it (its held time is not updated).
void IN_KeyUpCheat(kbutton_t *b)
{
	const char *c = Cmd_Argv(1);
	if (!c[0])
	{
		b->down[1] = 0;
		b->down[0] = 0;
		b->active = false;
		return;
	}
	int k = atoi(c);
	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;
	if (b->down[0] || b->down[1])
		return;
	b->active = false;
	unsigned int uptime = atoi(Cmd_Argv(2));
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += frame_msec / 2;
	b->active = false;
	b->val = 0.0f;
}

// 004EAAB0  client/cl_input.cpp:360
// The fraction of the frame the button was down, and resets it.
//
// The original returns 0 and 1 as constants and the fraction unrounded on the x87 stack; one division at the end
// in the returning block keeps the reconstruction's compiler from rounding it (both operands are whole frames'
// milliseconds, exact as floats) (see q_shared.cpp CMD_GetAnalogButtonValue). 0/1 and 1/1
// are those constants exactly.
X87_MATH float CL_KeyState(kbutton_t *key)
{
	int msec = key->msec;
	key->msec = 0;
	if (key->active)
	{
		if (!key->downtime)
			msec = com_frameTime;
		else
			msec += com_frameTime - key->downtime;
		key->downtime = com_frameTime;
	}
	float held;
	float frame;
	if (msec <= 0)
	{
		held = 0.0f;
		frame = 1.0f;
	}
	else if ((unsigned int)msec >= frame_msec)
	{
		held = 1.0f;
		frame = 1.0f;
	}
	else
	{
		assert_at(384, frame_msec);
		held = (float)msec;
		frame = (float)frame_msec;
	}
	return held / frame;
}

// 004EAB50  client/cl_input.cpp:390
void CL_SetStance(LocalClientNum_t localClientNum, StanceState stance)
{
	if (playersKb[localClientNum][KB_PRONE].active || playersKb[localClientNum][KB_DOWN].active)
		return;
	CL_GetLocalClientGlobals(localClientNum)->stance = stance;
}

// 004EAB90  client/cl_input.cpp:402
// Toggles between stance and standing; going prone while sprinting dives.
void CL_ToggleStance(LocalClientNum_t localClientNum, StanceState stance)
{
	if (playersKb[localClientNum][KB_PRONE].active || playersKb[localClientNum][KB_DOWN].active)
		return;
	if (CG_IsSprinting(localClientNum) && stance == CL_STANCE_PRONE)
		stance = CL_STANCE_DIVE_TO_PRONE;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	cl->stance = cl->stance == stance ? CL_STANCE_STAND : stance;
}

// 004EAC10  client/cl_input.cpp:419
// +gostand: stands up from crouch or prone, jumps when standing.
void IN_UpDown()
{
	IN_KeyDownCheat(&playersKb[0][KB_ANYUP]);
	if (playersKb[0][KB_PRONE].active || playersKb[0][KB_DOWN].active)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(LOCAL_CLIENT_0);
	if (cl->stance == CL_STANCE_CROUCH)
	{
		cl->stance = CL_STANCE_STAND;
	}
	else if (cl->stance == CL_STANCE_PRONE)
	{
		cl->stance = CL_STANCE_CROUCH;
	}
	else
	{
		assertt_at(440, 1, cl->stance == CL_STANCE_STAND);
		IN_KeyDownCheat(&playersKb[0][KB_UP]);
	}
}

// 004EAC90  client/cl_input.cpp:448
void IN_UpUp()
{
	IN_KeyUpCheat(&playersKb[0][KB_ANYUP]);
	IN_KeyUpCheat(&playersKb[0][KB_UP]);
}

// 004EACB0  client/cl_input.cpp:458
void IN_SpeedDown()
{
	CL_GetLocalClientGlobals(LOCAL_CLIENT_0)->usingAds = false;
	IN_KeyDownCheat(&playersKb[0][KB_SPEED]);
}

// 004EACD0  client/cl_input.cpp:472
void IN_SpeedUp()
{
	CL_GetLocalClientGlobals(LOCAL_CLIENT_0)->usingAds = false;
	IN_KeyUpCheat(&playersKb[0][KB_SPEED]);
}

// 004EACF0  client/cl_input.cpp:487
// Use released: a press shorter than cl_dblTapMaxHoldTime counts towards a double tap.
void IN_UpdateUseCount(LocalClientNum_t localClientNum)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	int maxHold = Dvar_GetInt(cl_dblTapMaxHoldTime);
	int held = com_frameTime - cl->useTime;
	cl->useHeld = false;
	if (held < maxHold)
		++cl->useCount;
	else
		cl->useCount = 0;
}

// 004EAD80  client/cl_input.cpp:509
// Use pressed: a gap longer than cl_dblTapMaxDelayTime ends a double tap.
void IN_UpdateUseHeld(LocalClientNum_t localClientNum)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED || cl->useHeld)
		return;
	if (com_frameTime - cl->useTime >= Dvar_GetInt(cl_dblTapMaxDelayTime))
		cl->useCount = 0;
	cl->useHeld = true;
	cl->useTime = com_frameTime;
}

// 004EAE10  client/cl_input.cpp:531
void IN_Activate_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_UpdateUseHeld(localClientNum);
	IN_KeyDown(&playersKb[localClientNum][KB_ACTIVATE], key, time, value);
}

// 004EAE80  client/cl_input.cpp:538
void IN_Activate_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_UpdateUseCount(localClientNum);
	IN_KeyUp(&playersKb[localClientNum][KB_ACTIVATE], key, time);
}

// 004EAF00  client/cl_input.cpp:545
void IN_Reload_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_UpdateUseHeld(localClientNum);
	IN_KeyDown(&playersKb[localClientNum][KB_RELOAD], key, time, value);
}

// 004EAF70  client/cl_input.cpp:552
void IN_Reload_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_UpdateUseCount(localClientNum);
	IN_KeyUp(&playersKb[localClientNum][KB_RELOAD], key, time);
}

// 004EAFF0  client/cl_input.cpp:559
void IN_UseReload_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_UpdateUseHeld(localClientNum);
	IN_KeyDown(&playersKb[localClientNum][KB_USE_RELOAD], key, time, value);
}

// 004EB060  client/cl_input.cpp:566
void IN_UseReload_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_UpdateUseCount(localClientNum);
	IN_KeyUp(&playersKb[localClientNum][KB_USE_RELOAD], key, time);
}

// 004EB0E0  client/cl_input.cpp:572
// Attack. A single-shot weapon fired faster than its iJamFireTime may jam: the chance grows with the rate, 60000/gap
// - 60000/iJamFireTime percent (shots a minute over the limit), clamped to 0-100, and the jam presses KB_JAM.
void IN_Attack_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	clientActive_t *client = CL_GetLocalClientGlobals(localClientNum);
	int now = Sys_Milliseconds();
	assert_at(576, client);
	if (client->snap.valid)
	{
		const WeaponDef *weapDef = BG_GetWeaponDef(client->snap.ps.weapon);
		if (weapDef && BG_GetFireType(client->snap.ps.weapon) == WEAPON_FIRETYPE_SINGLESHOT &&
		    !gAttackEdgeDetected[localClientNum])
		{
			int gap = now - client->lastFireTime;
			if (gap < weapDef->iJamFireTime)
			{
				int jamChance = (int)(60000.0f / gap - 60000.0f / weapDef->iJamFireTime);
				if (jamChance >= 100)
					jamChance = 100;
				else if (jamChance <= 0)
					jamChance = 0;
				if (now % 100 < jamChance)
					IN_KeyDown(&playersKb[localClientNum][KB_JAM], key, time, value);
			}
		}
		if (!gAttackEdgeDetected[localClientNum])
		{
			gAttackEdgeDetected[localClientNum] = true;
			client->lastFireTime = now;
		}
	}
	IN_KeyDown(&playersKb[localClientNum][KB_ATTACK], key, time, value);
}

// 004EB280  client/cl_input.cpp:614
void IN_Attack_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	gAttackEdgeDetected[localClientNum] = false;
	IN_KeyUp(&playersKb[localClientNum][KB_ATTACK], key, time);
	IN_KeyUp(&playersKb[localClientNum][KB_JAM], key, time);
}

// 004EB370  client/cl_input.cpp:621
void IN_Melee_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_KeyDown(&playersKb[localClientNum][KB_MELEE], key, time, value);
}

// 004EB3D0  client/cl_input.cpp:636
void IN_Melee_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_KeyUp(&playersKb[localClientNum][KB_MELEE], key, time);
}

// 004EB440  client/cl_input.cpp:641
void IN_Frag_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_KeyDown(&playersKb[localClientNum][KB_FRAG], key, time, value);
}

// 004EB4A0  client/cl_input.cpp:666
void IN_Frag_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_KeyUp(&playersKb[localClientNum][KB_FRAG], key, time);
}

// 004EB510  client/cl_input.cpp:674
void IN_Smoke_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	IN_KeyDown(&playersKb[localClientNum][KB_OFFHANDSECONDARY], key, time, value);
}

// 004EB570  client/cl_input.cpp:694
void IN_Smoke_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	IN_KeyUp(&playersKb[localClientNum][KB_OFFHANDSECONDARY], key, time);
}

// 004EB5E0  client/cl_input.cpp:699
// Stance: crouches (remembering the stance to return to if released quickly); while sprinting only marks the time;
// driving a vehicle, it is the vehicle's down key.
void IN_Stance_Down(LocalClientNum_t localClientNum, int key, unsigned int time, float value)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	IN_KeyDown(&playersKb[localClientNum][KB_STANCE], key, time, value);
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (cl->snap.ps.eFlags & EF_VEHICLE_DRIVER)
	{
		IN_KeyDown(&playersKb[localClientNum][KB_DOWN], key, time, value);
		return;
	}
	if (playersKb[localClientNum][KB_PRONE].active || playersKb[localClientNum][KB_DOWN].active)
		return;
	bool sprinting = CG_IsSprinting(localClientNum);
	cl->stanceHeld = true;
	if (sprinting)
	{
		cl->stanceTime = com_frameTime;
		return;
	}
	cl->stancePosition = cl->stance;
	cl->stanceTime = com_frameTime;
	if (cl->stance != CL_STANCE_CROUCH)
		cl->stance = CL_STANCE_CROUCH;
}

// 004EB730  client/cl_input.cpp:735
// Driving, releases KB_VEHICLEMOVEDOWN, not the KB_DOWN IN_Stance_Down pressed (B0014).
void IN_Stance_Up(LocalClientNum_t localClientNum, int key, unsigned int time)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	IN_KeyUp(&playersKb[localClientNum][KB_STANCE], key, time);
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (cl->snap.ps.eFlags & EF_VEHICLE_DRIVER)
	{
		IN_KeyUp(&playersKb[localClientNum][KB_VEHICLEMOVEDOWN], key, time);
		return;
	}
	if (playersKb[localClientNum][KB_PRONE].active || playersKb[localClientNum][KB_DOWN].active)
		return;
	if (CG_IsSprinting(localClientNum))
	{
		if (!BG_WeaponBlocksProne(cl->snap.ps.weapon))
			cl->stance = CL_STANCE_CROUCH;
		cl->stanceHeld = false;
	}
	if (cl->stanceHeld && cl->stancePosition == CL_STANCE_CROUCH)
		cl->stance = CL_STANCE_STAND;
	cl->stanceHeld = false;
}

// 004EB880  client/cl_input.cpp:789
void IN_WeapNextInventoryDown(LocalClientNum_t localClientNum)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	cl->weapNextHeld = true;
	cl->weapNextTime = com_frameTime;
}

// 004EB8D0  client/cl_input.cpp:802
void IN_WeapNextInventoryUp(LocalClientNum_t localClientNum)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (cl->weapNextHeld)
	{
		CG_CycleWeapon(localClientNum, 1);
		cl->weapNextHeld = false;
	}
}

// 004EB930  client/cl_input.cpp:819
// Toggles aiming down the sights on the edge of KB_THROW's analog value crossing 0.1.
void IN_ToggleADS(LocalClientNum_t localClientNum)
{
	T6_STATIC bool latch[1] T6_AT("IN_ToggleADS", "latch");
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (playersKb[localClientNum][KB_THROW].val < 0.1f)
	{
		latch[localClientNum] = false;
	}
	else if (!latch[localClientNum])
	{
		cl->usingAds = !cl->usingAds;
		latch[localClientNum] = true;
	}
}

// 004EB9B0  client/cl_input.cpp:845
void IN_CenterView(LocalClientNum_t localClientNum)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_CONNECTED)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	cl->viewangles.x = -cl->snap.ps.delta_angles.x;
}

// 004EBA10  client/cl_input.cpp:859
void IN_LowerStance()
{
	if (CL_GetLocalClientConnectionState(LOCAL_CLIENT_0) < CA_CONNECTED)
		return;
	if (playersKb[0][KB_PRONE].active || playersKb[0][KB_DOWN].active)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(LOCAL_CLIENT_0);
	if (cl->stance == CL_STANCE_STAND)
		cl->stance = CL_STANCE_CROUCH;
	else if (cl->stance == CL_STANCE_CROUCH)
		cl->stance = CL_STANCE_PRONE;
	else
		assertt_at(881, 1, cl->stance == CL_STANCE_PRONE);
}

// 004EBA90  client/cl_input.cpp:887
void IN_RaiseStance()
{
	if (CL_GetLocalClientConnectionState(LOCAL_CLIENT_0) < CA_CONNECTED)
		return;
	if (playersKb[0][KB_PRONE].active || playersKb[0][KB_DOWN].active)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(LOCAL_CLIENT_0);
	if (cl->stance == CL_STANCE_CROUCH)
		cl->stance = CL_STANCE_STAND;
	else if (cl->stance == CL_STANCE_PRONE)
		cl->stance = CL_STANCE_CROUCH;
	else
		assertt_at(909, 1, cl->stance == CL_STANCE_STAND);
}

// 004EBB10  client/cl_input.cpp:959
bool IN_IsTalkKeyHeld()
{
	return playersKb[0][KB_TALK].active;
}
