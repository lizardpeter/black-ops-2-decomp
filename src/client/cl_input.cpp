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
#include <string.h>

#include "bgame/bg_public.h"
#include "cgame_mp/cg_local_mp.h"
#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "universal/com_math.h"
#include "t6_compiler.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_shared.h"

// cgame

void CG_CycleWeapon(LocalClientNum_t localClientNum, int cycleForward);
int CG_AirburstLazeCancelledByMovement(float forwardMove, float rightMove, float pitchMove, float yawMove,
                                       float moveScale);
int CG_GetAirburstMarkDistance(LocalClientNum_t localClientNum, playerState_s *ps, bitarray<64> &buttons,
                               unsigned int *markDistance, int cancelled);

// cl_main.cpp, cl_keys.cpp, splitscreen.cpp
void CL_FinishMove(LocalClientNum_t localClientNum, usercmd_s *cmd);
void CL_ClampViewAngle(LocalClientNum_t localClientNum, int index, float min, float max);
void Key_ClearStates(LocalClientNum_t localClientNum);
bool Key_IsCatcherActive(LocalClientNum_t localClientNum, int mask);
bool CL_SplitscreenEnabled();
bool CL_IsFirstActiveLocalClient(LocalClientNum_t localClientNum);

// not reconstructed yet in this file
bool CG_HandleLocationSelectionInput(LocalClientNum_t localClientNum, usercmd_s *cmd);
void CL_CmdButtons(LocalClientNum_t localClientNum, usercmd_s *cmd);
void CL_GamepadMove(LocalClientNum_t localClientNum, usercmd_s *cmd, float frametime_base);
void CL_HandleSpecialStateInput(LocalClientNum_t localClientNum, bitarray<64> *buttons);

// qcommon, live, demo, gamepad
float Com_ViewScaleMsec(float msec);
ControllerIndex_t Com_LocalClient_GetControllerIndex(LocalClientNum_t localClientNum);
bool Com_SessionMode_IsZombiesGame();
void MSG_SetDefaultUserCmd(playerState_s *ps, usercmd_s *cmd);
bool Live_IsSystemUiActive();
bool Demo_IsThirdPersonCamera();
bool Demo_IsAnyFreeMoveCamera();
bool Demo_IsFreeCameraLockedOnEntity();
void Demo_ClampViewAngleForLockedOnEntity(LocalClientNum_t localClientNum);
bool GPad_IsActive(ControllerIndex_t portIndex);
void IN_Frame();
float AngleDelta(float a1, float a2);
float AngleNormalize360(float angle);

// game, aim assist, cgame
struct AimInput;
struct AimOutput;
enum profile_setting { PROFILE_MOUSE_SENSITIVITY = 0x29 };
float GamerProfile_GetFloat(profile_setting setting, ControllerIndex_t controllerIndex);
void AimAssist_UpdateMouseInput(const AimInput *input, AimOutput *output);
bool CG_ShouldUpdateViewAngles(LocalClientNum_t localClientNum);
bool GPad_InUse(LocalClientNum_t localClientNum);
bool Demo_IsPlaying();

// ui, devgui, win32
void UI_UpdateMouseScreenPosition(LocalClientNum_t localClientNum, int x, int y);
bool UI_KeysBypassMenu(LocalClientNum_t localClientNum);
bool UI_IsFocusedMenuUi3dWindow(LocalClientNum_t localClientNum);
void UI_MouseEvent(LocalClientNum_t localClientNum, int x, int y);
bool DevGui_IsActive();
void DevGui_MouseEvent(int dx, int dy);
void IN_SetCursorPos(int x, int y);
void IN_ShowSystemCursor(int show);

struct AimInput
{
	float deltaTime;
	float pitch;
	float pitchAxis;
	float pitchMax;
	float yaw;
	float yawAxis;
	float yawMax;
	float forwardAxis;
	float rightAxis;
	bitarray<64> button_bits;
	LocalClientNum_t localClientNum;
	const playerState_s *ps;
	float adsFrac;
};
static_assert(sizeof(void *) != 4 || sizeof(AimInput) == 0x38, "AimInput (the server's x86 layout)");

struct AimOutput
{
	float pitch;
	float yaw;
	float meleeChargeYaw;
	unsigned short meleeChargeEnt;
	unsigned char meleeChargeDist;
};

extern const dvar_t *cl_dblTapMaxHoldTime;   // universal/dvar.cpp
extern const dvar_t *cl_dblTapMaxDelayTime;  // universal/dvar.cpp
extern const dvar_t *cl_stanceHoldTime;      // universal/dvar.cpp
extern const dvar_t *cl_dtpHoldTime;         // universal/dvar.cpp
extern const dvar_t *cl_anglespeedkey;       // universal/dvar.cpp
extern const dvar_t *cl_yawspeed;            // universal/dvar.cpp
extern const dvar_t *cl_pitchspeed;          // universal/dvar.cpp
extern const dvar_t *m_filter;               // cl_main.cpp
extern const dvar_t *cl_paused;              // qcommon/common.cpp
extern const dvar_t *cl_mouseAccel;          // cl_main.cpp
extern const dvar_t *cl_freelook;            // cl_main.cpp
extern const dvar_t *m_pitch;                // cl_main.cpp
extern const dvar_t *m_yaw;                  // cl_main.cpp
extern const dvar_t *m_forward;              // cl_main.cpp
extern const dvar_t *m_side;                 // cl_main.cpp
extern const dvar_t *cl_showMouseRate;       // universal/dvar.cpp
extern const dvar_t *bg_useClientDamageKick; // bgame

T6_GLOBAL const dvar_t *cl_leanCountMin;
T6_GLOBAL int g_leaned;
T6_GLOBAL bool gAttackEdgeDetected[1];
T6_GLOBAL clientMouse_t mouseIn;

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
#ifdef T6_STORE
	// t6mp_msstore 140192650 tests "not below" (comiss standing, height; ja), so a NaN height counts as standing
	// height there: its compiler reordered the comparison (docs/store-build.md, NaN comparisons)
	return !((float)Dvar_GetInt(player_standingViewHeight) > ps->viewHeightCurrent) && (ps->pm_flags & PMF_SPRINTING);
#else
	return ps->viewHeightCurrent >= (float)Dvar_GetInt(player_standingViewHeight) && (ps->pm_flags & PMF_SPRINTING);
#endif
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

// Inlined everywhere (named by CL_StanceButtonUpdate's assert): a hold-to-prone or hold-to-crouch key is down.
static inline bool IN_IsTempStanceKeyActive(LocalClientNum_t localClientNum)
{
	return playersKb[localClientNum][KB_PRONE].active || playersKb[localClientNum][KB_DOWN].active;
}

// 004EAB50  client/cl_input.cpp:390
void CL_SetStance(LocalClientNum_t localClientNum, StanceState stance)
{
	if (IN_IsTempStanceKeyActive(localClientNum))
		return;
	CL_GetLocalClientGlobals(localClientNum)->stance = stance;
}

// 004EAB90  client/cl_input.cpp:402
// Toggles between stance and standing; going prone while sprinting dives.
void CL_ToggleStance(LocalClientNum_t localClientNum, StanceState stance)
{
	if (IN_IsTempStanceKeyActive(localClientNum))
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
	if (IN_IsTempStanceKeyActive(LOCAL_CLIENT_0))
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
	if (IN_IsTempStanceKeyActive(localClientNum))
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
	if (IN_IsTempStanceKeyActive(localClientNum))
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
	if (IN_IsTempStanceKeyActive(LOCAL_CLIENT_0))
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
	if (IN_IsTempStanceKeyActive(LOCAL_CLIENT_0))
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(LOCAL_CLIENT_0);
	if (cl->stance == CL_STANCE_CROUCH)
		cl->stance = CL_STANCE_STAND;
	else if (cl->stance == CL_STANCE_PRONE)
		cl->stance = CL_STANCE_CROUCH;
	else
		assertt_at(909, 1, cl->stance == CL_STANCE_STAND);
}

// 004ED810  client/cl_input.cpp:918
// The airburst rangefinder: while it marks a distance (CG_GetAirburstMarkDistance), the command carries it.
void CL_UpdateAirburstMarkDistance(LocalClientNum_t localClientNum, playerState_s *ps, usercmd_s *cmd,
                                   float forwardMove, float rightMove, float pitchMove, float yawMove,
                                   float moveScale)
{
	assert_at(922, cmd);
	unsigned int markDistance;
	if (CG_GetAirburstMarkDistance(localClientNum, ps, cmd->button_bits, &markDistance,
	                               CG_AirburstLazeCancelledByMovement(forwardMove, rightMove, pitchMove, yawMove,
	                                                                  moveScale)))
	{
		assert_at(929, markDistance <= (1 << sizeof( cmd->airburstMarkDistance ) * 8) - 1);
		cmd->airburstMarkDistance = markDistance;
		cmd->button_bits.setBit(BUTTON_AIRBURST_MARK);
	}
}

// 004EBB10  client/cl_input.cpp:959
bool IN_IsTalkKeyHeld()
{
	return playersKb[0][KB_TALK].active;
}

// 004EBB20  client/cl_input.cpp:1151
// Turning with keys. The keys' held fractions stay unrounded on the x87 stack into the angle arithmetic, as in the
// original (X87_MATH, one expression per angle change); the speeds are floats.
X87_MATH void CL_AdjustAngles(LocalClientNum_t localClientNum)
{
	kbutton_t *kb = playersKb[localClientNum];
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (cl->snap.ps.pm_flags & PMF_FROZEN)
		return;
	float speed;
	if (kb[KB_SPEED].active)
	{
		float scale = Dvar_GetFloat(cl_anglespeedkey);  // first: the product below stays on the x87 stack
		speed = cls.frametime * 0.001f * scale;
	}
	else
	{
		speed = cls.frametime * 0.001f;
	}
	float angleSpeed;
	if (!kb[KB_STRAFE].active)
	{
		if (cl->cgameMaxYawSpeed > 0.0f)
			angleSpeed = cl->cgameMaxYawSpeed * speed;
		else
			angleSpeed = Dvar_GetFloat(cl_yawspeed) * speed;
		cl->viewangles.y -= CL_KeyState(&kb[KB_RIGHT]) * angleSpeed;
		cl->viewangles.y += CL_KeyState(&kb[KB_LEFT]) * angleSpeed;
	}
	if (cl->cgameMaxPitchSpeed > 0.0f)
		angleSpeed = cl->cgameMaxPitchSpeed * speed;
	else
		angleSpeed = Dvar_GetFloat(cl_pitchspeed) * speed;
	cl->viewangles.x -= CL_KeyState(&kb[KB_LOOKUP]) * angleSpeed;
	cl->viewangles.x += CL_KeyState(&kb[KB_LOOKDOWN]) * angleSpeed;
}

// 004EBC70  client/cl_input.cpp:1196
// The stance key held: past cl_stanceHoldTime goes prone (or back up from prone); while sprinting, past
// cl_dtpHoldTime dives.
void CL_StanceButtonUpdate(LocalClientNum_t localClientNum)
{
	assert_at(1201, !IN_IsTempStanceKeyActive( localClientNum ));
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	if (!cl->stanceHeld)
		return;
	bool sprinting = CG_IsSprinting(localClientNum);
#ifdef T6_STORE
	// the Store revision holds for fixed times (t6mp_msstore CL_KeyMove 140192D50, this function inlined: 300 and
	// 200 ms) where the server reads cl_stanceHoldTime and cl_dtpHoldTime
	if (com_frameTime - cl->stanceTime < (sprinting ? 200 : 300))
		return;
#else
	if (com_frameTime - cl->stanceTime < Dvar_GetInt(sprinting ? cl_dtpHoldTime : cl_stanceHoldTime))
		return;
#endif
	if (sprinting)
	{
		cl->stance = CL_STANCE_DIVE_TO_PRONE;
	}
	else
	{
		cl->stance = cl->stancePosition != CL_STANCE_PRONE ? CL_STANCE_PRONE : CL_STANCE_STAND;
	}
	cl->stanceHeld = false;
}

// 004ED8E0  client/cl_input.cpp:1239
void CL_AddCurrentStanceToCmd(LocalClientNum_t localClientNum, usercmd_s *cmd)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	switch (cl->stance)
	{
	case CL_STANCE_CROUCH:
		cmd->button_bits.resetBit(BUTTON_PRONE);
		cmd->button_bits.setBit(BUTTON_CROUCH);
		cmd->button_bits.resetBit(BUTTON_STANCE_KEY);
		break;
	case CL_STANCE_DIVE_TO_PRONE:
		cmd->button_bits.setBit(BUTTON_DIVE_TO_PRONE);
		// fall through: diving ends prone
	case CL_STANCE_PRONE:
		cmd->button_bits.resetBit(BUTTON_CROUCH);
		cmd->button_bits.setBit(BUTTON_PRONE);
		cmd->button_bits.resetBit(BUTTON_STANCE_KEY);
		break;
	default:
		assert_at(1257, cl->stance == CL_STANCE_STAND);
		cmd->button_bits.resetBit(BUTTON_PRONE);
		cmd->button_bits.resetBit(BUTTON_CROUCH);
		cmd->button_bits.resetBit(BUTTON_STANCE_KEY);
		break;
	}
}

// 004ED970  client/cl_input.cpp:1269
// After a hold on weapnext (IN_WeapNextInventoryUp) chose the inventory weapon: ask for it until it's in hand.
void CL_WeapNextButtonUpdate(LocalClientNum_t localClientNum, usercmd_s *cmd)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	cg_t *cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	if (!cl->switchingToInventory)
		return;
	if (cgameGlob->weaponSelect.weaponData == cl->snap.ps.inventoryWeapon.weaponData &&
	    cl->snap.ps.weapon.weaponData != cgameGlob->weaponSelect.weaponData)
	{
		cmd->button_bits.setBit(BUTTON_SWITCH_TO_INVENTORY);
		return;
	}
	cl->switchingToInventory = false;
}

// 004EDA00  client/cl_input.cpp:1314
// The movement keys and buttons into the command. Each key's held fraction is scaled to +-127 and truncated from the
// x87 stack (X87_MATH: one expression per key).
X87_MATH void CL_KeyMove(LocalClientNum_t localClientNum, usercmd_s *cmd)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	kbutton_t *kb = playersKb[localClientNum];
	if (cl->snap.ps.eFlags & EF_VEHICLE_DRIVER)
	{
		cl->wasInVehicle = 1;
	}
	else if (cl->wasInVehicle)
	{
		// leaving a vehicle lets go of the down button the stance key held there (but see B0014)
		kb[KB_DOWN].active = false;
		kb[KB_DOWN].down[1] = 0;
		kb[KB_DOWN].down[0] = 0;
		cl->wasInVehicle = 0;
	}
	if (kb[KB_PRONE].active)
	{
		cmd->button_bits.resetBit(BUTTON_CROUCH);
		cmd->button_bits.setBit(BUTTON_PRONE);
		cmd->button_bits.setBit(BUTTON_STANCE_KEY);
	}
	else if (kb[KB_DOWN].active)
	{
		cmd->button_bits.resetBit(BUTTON_PRONE);
		cmd->button_bits.setBit(BUTTON_CROUCH);
		cmd->button_bits.setBit(BUTTON_STANCE_KEY);
	}
	else
	{
		CL_StanceButtonUpdate(localClientNum);
		CL_AddCurrentStanceToCmd(localClientNum, cmd);
	}
	CL_WeapNextButtonUpdate(localClientNum, cmd);
	clientActive_t *client = CL_GetLocalClientGlobals(localClientNum);
	if (client->useCount == 2)
	{
		cmd->button_bits.setBit(BUTTON_USE_DOUBLE_TAP);
		client->useCount = 0;
		client->useTime = 0;
	}
	if (kb[KB_SPEED].active == !cl->usingAds)
		cmd->button_bits.setBit(BUTTON_ADS);
	else
		cmd->button_bits.resetBit(BUTTON_ADS);
	int side = (int)(CL_KeyState(&kb[KB_MOVERIGHT]) * 127.0f);
	side += (int)(CL_KeyState(&kb[KB_MOVELEFT]) * -127.0f);
	int forward = (int)(CL_KeyState(&kb[KB_FORWARD]) * 127.0f);
	forward += (int)(CL_KeyState(&kb[KB_BACK]) * -127.0f);
	if (!kb[KB_BACK].active)
	{
		if (kb[KB_SPRINT].active || kb[KB_SPRINT].wasPressed)
		{
			cmd->button_bits.setBit(BUTTON_SPRINT);
			kb[KB_SPRINT].wasPressed = false;
		}
		else
		{
			cmd->button_bits.resetBit(BUTTON_SPRINT);
		}
	}
	if (kb[KB_STRAFE].active && !cmd->button_bits.testBit(BUTTON_SPRINT))
	{
		side += (int)(CL_KeyState(&kb[KB_RIGHT]) * 127.0f);
		side += (int)(CL_KeyState(&kb[KB_LEFT]) * -127.0f);
	}
	cmd->forwardmove = ClampChar(forward);
	cmd->rightmove = ClampChar(side);
	if (kb[KB_LOOKUP].active || kb[KB_LOOKDOWN].active)
	{
		int pitch = 0;
		if (kb[KB_LOOKUP].active)
			pitch = -127;
		if (kb[KB_LOOKDOWN].active)
			pitch += 127;
		cmd->pitchmove = ClampChar(pitch);
	}
	if (kb[KB_RIGHT].active || kb[KB_LEFT].active)
	{
		int yaw = 0;
		if (kb[KB_RIGHT].active)
			yaw = -127;
		if (kb[KB_LEFT].active)
			yaw += 127;
		cmd->yawmove = ClampChar(yaw);
	}
}

// 004EDC60  client/cl_input.cpp:1554
// Mouse motion: to the dev GUI, to a menu that takes the mouse, or accumulated for CL_MouseMove. Returns false when a
// menu took it.
int CL_MouseEvent(LocalClientNum_t localClientNum, int x, int y, int dx, int dy)
{
	if (DevGui_IsActive())
	{
		DevGui_MouseEvent(dx, dy);
		return 1;
	}
	clientUIActive_t *clientUIActive = CL_GetLocalClientUIGlobals(localClientNum);
	UI_UpdateMouseScreenPosition(localClientNum, x, y);
	if ((dx || dy) && (clientUIActive->keyCatchers & 0x48) && !UI_KeysBypassMenu(localClientNum) &&
	    !UI_IsFocusedMenuUi3dWindow(localClientNum))
	{
		UI_MouseEvent(localClientNum, x, y);
		return 0;
	}
	IN_ShowSystemCursor(0);
	mouseIn.mouseDx[mouseIn.mouseIndex] += dx;
	mouseIn.mouseDy[mouseIn.mouseIndex] += dy;
	return 1;
}

// 004EBD70  client/cl_input.cpp:1641
// This frame's mouse movement (with m_filter, averaged with the last frame's), and starts the next frame's.
void CL_GetMouseMovement(clientActive_t *cl, float *mx, float *my)
{
	assert_at(1642, mx);
	assert_at(1643, my);
	if (Dvar_GetBool(m_filter))
	{
		*mx = (mouseIn.mouseDx[0] + mouseIn.mouseDx[1]) * 0.5f;
		*my = (mouseIn.mouseDy[0] + mouseIn.mouseDy[1]) * 0.5f;
	}
	else
	{
		*mx = (float)mouseIn.mouseDx[mouseIn.mouseIndex];
		*my = (float)mouseIn.mouseDy[mouseIn.mouseIndex];
	}
	mouseIn.mouseIndex ^= 1;
	mouseIn.mouseDx[mouseIn.mouseIndex] = 0;
	mouseIn.mouseDy[mouseIn.mouseIndex] = 0;
}

// 004EBE80  client/cl_input.cpp:1661
void SetMeleeChargeCmd(const AimOutput *aimOutput, usercmd_s *cmd)
{
	assert_at(1662, aimOutput);
	assert_at(1663, cmd);
	cmd->meleeChargeEnt = aimOutput->meleeChargeEnt;
	cmd->meleeChargeDist = aimOutput->meleeChargeDist;
}

// 004EBEE0  client/cl_input.cpp:1676
void CL_SetCursorPos(int x, int y)
{
	IN_SetCursorPos(x, y);
}

// 004EBEF0  client/cl_input.cpp:1684
void CL_ShowSystemCursor(int show)
{
	IN_ShowSystemCursor(show);
}

// Mouse movement into move keys: VC100 adds 2^-30 to the (float) value and rounds to nearest on the x87 stack, so a
// half rounds up and anything else to the nearer integer. In double precision, as the original's x87 runs.
static inline int CL_MouseMoveToInt(float move)
{
	typedef double v2df __attribute__((__vector_size__(16)));
	v2df v = {move + 9.313225746154785e-10, 0.0};  // cvtsd2si: round to nearest, as fistp
	return __builtin_ia32_cvtsd2si(v);
}

// 004EEF00  client/cl_input.cpp:1710
// Mouse look. The movement (CL_GetMouseMovement) is scaled by the profile's sensitivity plus cl_mouseAccel times the
// mouse rate (counts per millisecond), and by the cgame's FOV scale. Turning with m_yaw/m_pitch degrees per count,
// capped per frame at the cgame's turn rate; strafing (+strafe) or without mouse look, the movement walks instead.
// Aim assist then gets the angles. The server's x87 chains run here in x87_t (t6_compiler.h).
void CL_MouseMove(LocalClientNum_t localClientNum, usercmd_s *cmd, float frametime_base)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	float oldYaw = cl->viewangles.y;
	float oldPitch = cl->viewangles.x;
	float mx;
	float my;
	CL_GetMouseMovement(cl, &mx, &my);
	if (!frame_msec)
		return;
	float rate = (float)(sqrt_x87((x87_t)mx * mx + (x87_t)my * my) / frame_msec);
	float sensitivity = GamerProfile_GetFloat(PROFILE_MOUSE_SENSITIVITY,
	                                          Com_LocalClient_GetControllerIndex(localClientNum));
	float accelSensitivity =
		(float)(((x87_t)Dvar_GetFloat(cl_mouseAccel) * rate + sensitivity) * cl->cgameFOVSensitivityScale);
	if (rate != 0.0f && Dvar_GetBool(cl_showMouseRate))
		Com_Printf(25, "%f : %f\n", (double)rate, (double)accelSensitivity);
	if (cl->snap.ps.pm_flags & PMF_FROZEN)
		return;
	if (cl->snap.ps.weaponstate == WEAPON_DEPLOYED)
		return;
	mx *= accelSensitivity;
	my *= accelSensitivity;
	if ((cl->snap.ps.otherFlags & POF_NO_MOVE_INPUT) && !Demo_IsPlaying())
	{
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->pitchmove = 0;
		cmd->yawmove = 0;
	}
	else
	{
		if (mx != 0.0f || my != 0.0f)
		{
			kbutton_t *kb = playersKb[localClientNum];
			int yawMove = 0;
			int pitchMove = 0;
			// linked to an entity (and not a link that keeps its own view): the turn cap is at most 360 a second
			bool bLinkedView = cl->snap.ps.pm_type == PM_NORMAL_LINKED && !(cl->snap.ps.linkFlags & 1);
			if (kb[KB_STRAFE].active)
			{
				float side = Dvar_GetFloat(m_side) * mx;
				cmd->rightmove = ClampChar(cmd->rightmove + CL_MouseMoveToInt(side));
			}
			else
			{
				float yaw = Dvar_GetFloat(m_yaw) * mx;
				float cap = cl->cgameMaxYawSpeed;
				if (bLinkedView && (cap <= 0.0f || 360.0f - cap < 0.0f))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					cap = (float)((x87_t)frame_msec * cap * 0.001f);
					yaw = I_fmax(-cap, I_fmin(cap, yaw));
				}
				cl->viewangles.y -= yaw;
				yawMove = -(int)I_fmin(mx, 127.0f);
			}
			if ((!kb[KB_MLOOK].active && !Dvar_GetBool(cl_freelook)) || kb[KB_STRAFE].active)
			{
				my = Dvar_GetFloat(m_forward) * my;
				cmd->forwardmove = ClampChar(cmd->forwardmove - CL_MouseMoveToInt(my));
			}
			else
			{
				float pitch = Dvar_GetFloat(m_pitch) * my;
				float cap = cl->cgameMaxPitchSpeed;
				if (bLinkedView && (cap <= 0.0f || 360.0f - cap < 0.0f))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					cap = (float)((x87_t)frame_msec * cap * 0.001f);
					pitch = I_fmax(-cap, I_fmin(cap, pitch));
				}
				cl->viewangles.x += pitch;
				int sign = Dvar_GetFloat(m_pitch) > 0.0f ? 1 : -1;
				pitchMove = (int)(I_fmin(my, 127.0f) * sign);
			}
			cmd->pitchmove = ClampChar(cmd->pitchmove + pitchMove);
			cmd->yawmove = ClampChar(cmd->yawmove + yawMove);
		}
		if (!Demo_IsPlaying() && (cl->snap.ps.eFlags2 & EF2_VIEW_LOCKED))
		{
			// the view may not turn: put it back
			cl->viewangles.y = oldYaw;
			cl->viewangles.x = oldPitch;
			return;
		}
	}
	if (!GPad_InUse(localClientNum) && CG_ShouldUpdateViewAngles(localClientNum))
	{
		// ps and adsFrac are left unset, as in the original
		AimInput aimInput;
		aimInput.deltaTime = frametime_base;
		aimInput.pitch = cl->viewangles.x;
		aimInput.pitchAxis = 0.0f;
		aimInput.pitchMax = 0.0f;
		aimInput.yaw = cl->viewangles.y;
		aimInput.yawAxis = 0.0f;
		aimInput.yawMax = 0.0f;
		aimInput.forwardAxis = 0.0f;
		aimInput.rightAxis = 0.0f;
		aimInput.button_bits = cmd->button_bits;
		aimInput.localClientNum = LOCAL_CLIENT_0;
		AimOutput aimOutput;
		AimAssist_UpdateMouseInput(&aimInput, &aimOutput);
		cl->viewangles.x = aimOutput.pitch;
		cl->viewangles.y = aimOutput.yaw;
		SetMeleeChargeCmd(&aimOutput, cmd);
	}
	CL_UpdateAirburstMarkDistance(localClientNum, &cl->snap.ps, cmd, cmd->forwardmove, cmd->rightmove,
	                              cmd->pitchmove, cmd->yawmove, 127.0f);
}

// 004EBF00  client/cl_input.cpp:2783
// The damage kick, as short angles (truncated, not wrapped as ANGLE2SHORT would).
void CL_CopyDamageKickAngles(LocalClientNum_t localClientNum, usercmd_s *cmd)
{
	cg_t *cgameGlob = CG_GetLocalClientGlobals(localClientNum);
	cmd->damageKick[0] = (short)(int)(cgameGlob->damageKickAngles.x * 182.04445f);
	cmd->damageKick[1] = (short)(int)(cgameGlob->damageKickAngles.y * 182.04445f);
}

// 004EF530  client/cl_input.cpp:2794
// One frame's command. The view's pitch may not move more than 90 degrees in a frame. X87_MATH: AngleDelta's result
// is compared unrounded, as in the original.
X87_MATH usercmd_s CL_CreateCmd(LocalClientNum_t localClientNum)
{
	usercmd_s cmd;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	float oldPitch = cl->viewangles.x;
	CL_AdjustAngles(localClientNum);
	memset(&cmd, 0, sizeof(cmd));
	if (Live_IsSystemUiActive())
	{
		Key_ClearStates(localClientNum);
		CL_AddCurrentStanceToCmd(localClientNum, &cmd);
		CL_FinishMove(localClientNum, &cmd);
		return cmd;
	}
	if (!Key_IsCatcherActive(localClientNum, 4) || !CG_HandleLocationSelectionInput(localClientNum, &cmd))
	{
		CL_CmdButtons(localClientNum, &cmd);
		CL_KeyMove(localClientNum, &cmd);
		float frametime_base = Com_ViewScaleMsec(cls.frametime_base);
		CL_MouseMove(localClientNum, &cmd, frametime_base);
		if (GPad_IsActive(Com_LocalClient_GetControllerIndex(localClientNum)))
			CL_GamepadMove(localClientNum, &cmd, frametime_base);
		CL_HandleSpecialStateInput(localClientNum, &cmd.button_bits);
		if (AngleDelta(cl->viewangles.x, oldPitch) > 90.0f)
			cl->viewangles.x = AngleNormalize360(oldPitch + 90.0f);
		else if (AngleDelta(oldPitch, cl->viewangles.x) > 90.0f)
			cl->viewangles.x = AngleNormalize360(oldPitch - 90.0f);
		if (Demo_IsThirdPersonCamera())
			CL_ClampViewAngle(localClientNum, 0, -90.0f, 90.0f);
		if (Demo_IsAnyFreeMoveCamera())
		{
			if (Demo_IsFreeCameraLockedOnEntity())
			{
				Demo_ClampViewAngleForLockedOnEntity(localClientNum);
			}
			else
			{
				if (cl->viewangles.x > 180.0f)
					cl->viewangles.x -= 360.0f;
				CL_ClampViewAngle(localClientNum, 0, -88.0f, 88.0f);
			}
		}
		if (Dvar_GetBool(bg_useClientDamageKick))
			CL_CopyDamageKickAngles(localClientNum, &cmd);
	}
	CL_FinishMove(localClientNum, &cmd);
	return cmd;
}

// 004EF7A0  client/cl_input.cpp:2905
// One new command per call (CL_Input calls it once a client frame) into the 128-command ring. B0001: the "connection
// interrupted" warning counts these, so it depends on the frame rate.
void CL_CreateNewCommands(LocalClientNum_t localClientNum)
{
	if (CL_GetLocalClientConnectionState(localClientNum) < CA_PRIMED)
		return;
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	++cl->cmdNumber;
	int cmdNum = cl->cmdNumber & 0x7F;
	if (CL_GetLocalClientMigrationState(localClientNum) != CMSTATE_INACTIVE &&
	    CL_GetLocalClientMigrationState(localClientNum) != CMSTATE_NEWHOSTCONNECT)
	{
		// the host is migrating: send a neutral command
		MSG_SetDefaultUserCmd(&cl->snap.ps, &cl->cmds[cmdNum]);
		return;
	}
	cl->cmds[cmdNum] = CL_CreateCmd(localClientNum);
}

// 004EF8C0  client/cl_input.cpp:3318
void CL_Input(LocalClientNum_t localClientNum)
{
	if (CL_GetLocalClientConnectionState(localClientNum) != CA_ACTIVE)
		return;
	if (Dvar_GetInt(cl_paused) == 1 && CL_SplitscreenEnabled() && Com_SessionMode_IsZombiesGame())
		return;
	if (CL_IsFirstActiveLocalClient(localClientNum))
	{
		cls.inputUpdatedPrevFrame = true;
		IN_Frame();
	}
	CL_CreateNewCommands(localClientNum);
}
