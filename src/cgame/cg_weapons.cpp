// cgame/cg_weapons.cpp (CoDMPServer_PC: cg_weapons.obj)
//
// The client's weapons. So far the airburst rangefinder's movement check, which CL_UpdateAirburstMarkDistance calls
// (and the Store build inlines there).
#define T6_SOURCE "cgame/cg_weapons.cpp"

#include "universal/dvar.h"

#include <math.h>

extern const dvar_t *cg_airburstStrafeRestriction;  // cgame_mp/cg_main_mp.cpp
extern const dvar_t *cg_airburstCameraRestriction;

// 004DDCC0  cgame/cg_weapons.cpp:11726
// Lasing a distance is cancelled by moving (walk and strafe) or turning (pitch and yaw) faster than the dvars allow,
// each measured as the length of its two movements, scaled to the move keys' range.
int CG_AirburstLazeCancelledByMovement(float forwardMove, float rightMove, float pitchMove, float yawMove,
                                       float moveScale)
{
	float strafeRestriction = Dvar_GetFloat(cg_airburstStrafeRestriction);
	float cameraRestriction = Dvar_GetFloat(cg_airburstCameraRestriction);
	float scale = 1.0f / moveScale;
	float forward = scale * forwardMove;
	float right = scale * rightMove;
	float pitch = scale * pitchMove;
	float yaw = scale * yawMove;
	float strafe = sqrtf(right * right + forward * forward);
	float camera = sqrtf(yaw * yaw + pitch * pitch);
	int cancelled = 0;
	if (strafe > strafeRestriction)
		cancelled = 1;
	if (camera > cameraRestriction)
		cancelled = 1;
	return cancelled;
}
