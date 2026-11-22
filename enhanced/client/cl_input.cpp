// enhanced/client/cl_input.cpp: mouse look without acceleration or smoothing (P0002; B0017, B0018), in MW2019's
// units (P0003).
//
// CL_MouseMove as in the Store build (src/client/cl_input.cpp, its T6_STORE branches), with these changes:
// - MW2019 sensitivity (P0003): one count turns the view by m_sens * 0.0066 degrees, at any field of view, hip fire
//   and aiming down sights alike. The profile's sensitivity (the menu's slider), the cgame's FOV scale, m_yaw and
//   m_pitch's size have no effect; m_pitch's sign still inverts the pitch (the menu's option), and 0 still stops it.
//   The moves the mouse makes with +strafe or without free look, and the commands' pitchmove and yawmove, use the
//   counts scaled as the original's at m_yaw 0.022 (m_sens * 0.3).
// - The view angles are kept in double precision between calls (P0003), so the same movement over more frames
//   (higher polling rates and frame rates) gives the same angle. Anything else that changes them (spawning, the
//   cgame's limits, aim assist) is picked up the next time.
// - No acceleration (P0002): cl_mouseAccel has no effect.
// - No smoothing (P0002): CL_GetMouseMovement takes this frame's movement only; m_filter has no effect.
// - A frame of 0 ms (possible at 1000 FPS and above) keeps the movement for the next frame, where the original took
//   it and dropped it (B0017).
// - The yaw is brought back within [-180, 180) at the end, so its precision doesn't wear down after many turns the
//   same way (B0018); the commands' angles are the same, since a short angle wraps at 360.
// The rest (turn caps, strafing with +strafe, aim assist's auto-melee, the airburst sight) is as the original.
// Store build only.
#ifdef T6_STORE
#define T6_SOURCE "client/cl_input.cpp"

#include "client/client.h"

#include "aim_assist/aim_assist.h"
#include "bgame/bg_public.h"
#include "demo/demo.h"
#include "game/gamerprofile.h"
#include "qcommon/qcommon.h"
#include "t6_static.h"
#include "universal/com_math.h"
#include "universal/dvar.h"
#include "win32/win_gamepad.h"

#include <math.h>

bool CG_ShouldUpdateViewAngles(LocalClientNum_t localClientNum);
void SetMeleeChargeCmd(const AimOutput *aimOutput, usercmd_s *cmd);
void CL_UpdateAirburstMarkDistance(LocalClientNum_t localClientNum, playerState_s *ps, usercmd_s *cmd,
                                   float forwardmove, float rightmove, float pitchmove, float yawmove,
                                   float moveScale);

extern const dvar_t *cl_freelook;
const dvar_t *m_sens;  // MW2019 units: registered on first use (CL_MouseSensitivity)
extern const dvar_t *m_pitch;
extern const dvar_t *m_forward;
extern const dvar_t *m_side;

T6_GLOBAL clientMouse_t mouseIn;
T6_STATIC kbutton_t playersKb[1][NUM_BUTTONS] T6_AT("", "playersKb");

// This frame's movement and the start of the next frame's; no averaging with the last frame (m_filter).
void CL_GetMouseMovement(clientActive_t *cl, float *mx, float *my)
{
	*mx = (float)mouseIn.mouseDx[mouseIn.mouseIndex];
	*my = (float)mouseIn.mouseDy[mouseIn.mouseIndex];
	mouseIn.mouseIndex ^= 1;
	mouseIn.mouseDx[mouseIn.mouseIndex] = 0;
	mouseIn.mouseDy[mouseIn.mouseIndex] = 0;
}

// MW2019's mouse scale: degrees per count per unit of m_sens
#define MOUSE_DEGREES_PER_COUNT 0.0066
// The original's turn per scaled count at its default m_yaw: the moves and commands take the counts scaled to it
#define MOUSE_ORIGINAL_YAW 0.022

// m_sens, registered the first time the mouse moves the view (the dvar system is up by then). A value the config
// set before that (seta m_sens ...) is kept: registering a dvar that already exists takes its value.
static float CL_MouseSensitivity()
{
	if (!m_sens)
		m_sens = _Dvar_RegisterFloat("m_sens", 1.5f, 0.01f, 100.0f, 1,
		                             "Mouse sensitivity in MW2019 units: 0.0066 degrees per count each, at any "
		                             "field of view");
	return Dvar_GetFloat(m_sens);
}

// The view angles (pitch, yaw) as this file last wrote them, in double precision. A float that differs from the
// one written was changed elsewhere: it is taken as it is.
static double s_viewAngles[2];
static float s_viewAnglesWritten[2];

static double CL_ViewAngle(clientActive_t *cl, int axis)
{
	if (!(cl->viewangles[axis] == s_viewAnglesWritten[axis]))
		s_viewAngles[axis] = cl->viewangles[axis];
	return s_viewAngles[axis];
}

static void CL_SetViewAngle(clientActive_t *cl, int axis, double angle)
{
	s_viewAngles[axis] = angle;
	s_viewAnglesWritten[axis] = (float)angle;
	cl->viewangles[axis] = s_viewAnglesWritten[axis];
}

// The view's yaw back into [-180, 180) by whole turns (B0018): exact, and the commands' short angles don't change,
// since they wrap at 360.
static void CL_WrapViewYaw(clientActive_t *cl)
{
	double yaw = CL_ViewAngle(cl, 1);
	if (yaw >= 180.0 || yaw < -180.0)
	{
		yaw = fmod(yaw, 360.0);  // exact: the remainder of two doubles is representable
		if (yaw >= 180.0)
			yaw -= 360.0;
		else if (yaw < -180.0)
			yaw += 360.0;
		CL_SetViewAngle(cl, 1, yaw);
	}
}

// The Store's /fp:fast forms of these comparisons, as in the faithful CL_MouseMove
#define MOUSE_CAP_OUT_OF_RANGE(cap) (!((cap) > 0.0f && (cap) <= 360.0f))
#define MOUSE_MIN_127(v) ((v) > 127.0f ? 127.0f : (v))

void CL_MouseMove(LocalClientNum_t localClientNum, usercmd_s *cmd, float frametime_base)
{
	clientActive_t *cl = CL_GetLocalClientGlobals(localClientNum);
	float oldYaw = cl->viewangles.y;
	float oldPitch = cl->viewangles.x;
	if (!frame_msec)
		return;  // the movement waits for the next frame (B0017)
	float countsX;
	float countsY;
	CL_GetMouseMovement(cl, &countsX, &countsY);
	float sensitivity = CL_MouseSensitivity();
	if (cl->snap.ps.pm_flags & PMF_FROZEN)
		return;
	if (cl->snap.ps.weaponstate == WEAPON_DEPLOYED)
		return;
	double degreesPerCount = (double)sensitivity * MOUSE_DEGREES_PER_COUNT;
	float scale = sensitivity * (float)(MOUSE_DEGREES_PER_COUNT / MOUSE_ORIGINAL_YAW);
	float mx = countsX * scale;
	float my = countsY * scale;
	if ((cl->snap.ps.otherFlags & POF_NO_MOVE_INPUT) && !Demo_IsPlaying())
	{
		cmd->forwardmove = 0;
		cmd->rightmove = 0;
		cmd->pitchmove = 0;
		cmd->yawmove = 0;
	}
	else
	{
		if (mx < 0.0f || mx > 0.0f || my < 0.0f || my > 0.0f)
		{
			kbutton_t *kb = playersKb[localClientNum];
			int yawMove = 0;
			int pitchMove = 0;
			bool bLinkedView = cl->snap.ps.pm_type == PM_NORMAL_LINKED && !(cl->snap.ps.linkFlags & 1);
			if (kb[KB_STRAFE].active)
			{
				float side = Dvar_GetFloat(m_side) * mx;
				cmd->rightmove = ClampChar(cmd->rightmove + (int)floorf(side + 0.5f));
			}
			else
			{
				double yaw = countsX * degreesPerCount;
				float cap = cl->cgameMaxYawSpeed;
				if (bLinkedView && MOUSE_CAP_OUT_OF_RANGE(cap))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					double limit = (double)frame_msec * cap * 0.001;  // a limit the game sets (vehicles, turrets)
					yaw = yaw > limit ? limit : yaw < -limit ? -limit : yaw;
				}
				CL_SetViewAngle(cl, 1, CL_ViewAngle(cl, 1) - yaw);
				yawMove = -(int)MOUSE_MIN_127(mx);
			}
			if ((!kb[KB_MLOOK].active && !Dvar_GetBool(cl_freelook)) || kb[KB_STRAFE].active)
			{
				my = Dvar_GetFloat(m_forward) * my;
				cmd->forwardmove = ClampChar(cmd->forwardmove - (int)floorf(my + 0.5f));
			}
			else
			{
				float pitchScale = Dvar_GetFloat(m_pitch);  // its sign: inverted (the menu's option), or 0: no pitch
				double pitch = countsY * degreesPerCount * (pitchScale > 0.0f ? 1.0 : pitchScale < 0.0f ? -1.0 : 0.0);
				float cap = cl->cgameMaxPitchSpeed;
				if (bLinkedView && MOUSE_CAP_OUT_OF_RANGE(cap))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					double limit = (double)frame_msec * cap * 0.001;
					pitch = pitch > limit ? limit : pitch < -limit ? -limit : pitch;
				}
				CL_SetViewAngle(cl, 0, CL_ViewAngle(cl, 0) + pitch);
				int sign = pitchScale > 0.0f ? 1 : -1;
				pitchMove = (int)(MOUSE_MIN_127(my) * sign);
			}
			cmd->pitchmove = ClampChar(cmd->pitchmove + pitchMove);
			cmd->yawmove = ClampChar(cmd->yawmove + yawMove);
		}
		if (!Demo_IsPlaying() && (cl->snap.ps.eFlags2 & EF2_VIEW_LOCKED))
		{
			cl->viewangles.y = oldYaw;
			cl->viewangles.x = oldPitch;
			CL_WrapViewYaw(cl);
			return;
		}
	}
	if (!GPad_InUse(localClientNum) && CG_ShouldUpdateViewAngles(localClientNum))
	{
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
	CL_WrapViewYaw(cl);  // after aim assist, which may turn the view too
	CL_UpdateAirburstMarkDistance(localClientNum, &cl->snap.ps, cmd, cmd->forwardmove, cmd->rightmove,
	                              cmd->pitchmove, cmd->yawmove, 127.0f);
}
#endif
