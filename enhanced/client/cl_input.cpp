// enhanced/client/cl_input.cpp: mouse look without acceleration or smoothing (P0002; B0017, B0018).
//
// CL_MouseMove as in the Store build (src/client/cl_input.cpp, its T6_STORE branches), with four changes:
// - no acceleration: the movement is scaled by the profile's sensitivity and the cgame's FOV scale (aiming down
//   sights) only; cl_mouseAccel has no effect;
// - no smoothing: CL_GetMouseMovement takes this frame's movement only; m_filter has no effect;
// - a frame of 0 ms (possible at 1000 FPS and above) keeps the movement for the next frame, where the original took
//   it and dropped it (B0017);
// - the yaw is brought back within [-180, 180) at the end, so its float precision doesn't wear down after many turns
//   the same way (B0018); the commands' angles are the same, since a short angle wraps at 360.
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
extern const dvar_t *m_pitch;
extern const dvar_t *m_yaw;
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

// The view's yaw back into [-180, 180) by whole turns (B0018): exact in float, and the commands' short angles
// don't change, since they wrap at 360.
static void CL_WrapViewYaw(clientActive_t *cl)
{
	float yaw = cl->viewangles.y;
	if (yaw >= 180.0f || yaw < -180.0f)
	{
		yaw = fmodf(yaw, 360.0f);  // exact: the remainder of two floats is representable
		if (yaw >= 180.0f)
			yaw -= 360.0f;
		else if (yaw < -180.0f)
			yaw += 360.0f;
		cl->viewangles.y = yaw;
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
	float mx;
	float my;
	CL_GetMouseMovement(cl, &mx, &my);
	float sensitivity = GamerProfile_GetFloat(PROFILE_MOUSE_SENSITIVITY,
	                                          Com_LocalClient_GetControllerIndex(localClientNum));
	float scale = sensitivity * cl->cgameFOVSensitivityScale;
	if (cl->snap.ps.pm_flags & PMF_FROZEN)
		return;
	if (cl->snap.ps.weaponstate == WEAPON_DEPLOYED)
		return;
	mx *= scale;
	my *= scale;
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
				float yaw = Dvar_GetFloat(m_yaw) * mx;
				float cap = cl->cgameMaxYawSpeed;
				if (bLinkedView && MOUSE_CAP_OUT_OF_RANGE(cap))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					cap = (float)frame_msec * cap * 0.001f;
					yaw = I_fmax(-cap, I_fmin(cap, yaw));
				}
				cl->viewangles.y -= yaw;
				yawMove = -(int)MOUSE_MIN_127(mx);
			}
			if ((!kb[KB_MLOOK].active && !Dvar_GetBool(cl_freelook)) || kb[KB_STRAFE].active)
			{
				my = Dvar_GetFloat(m_forward) * my;
				cmd->forwardmove = ClampChar(cmd->forwardmove - (int)floorf(my + 0.5f));
			}
			else
			{
				float pitch = Dvar_GetFloat(m_pitch) * my;
				float cap = cl->cgameMaxPitchSpeed;
				if (bLinkedView && MOUSE_CAP_OUT_OF_RANGE(cap))
					cap = 360.0f;
				if (cap > 0.0f)
				{
					cap = (float)frame_msec * cap * 0.001f;
					pitch = I_fmax(-cap, I_fmin(cap, pitch));
				}
				cl->viewangles.x += pitch;
				int sign = Dvar_GetFloat(m_pitch) > 0.0f ? 1 : -1;
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
