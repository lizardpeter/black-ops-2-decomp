// demo/demo_camera.cpp (CoDMPServer_PC: demo_camera.obj)
//
// The theater's cameras. So far the mode tests and the pitch limits the client's command code applies (all inlined in
// the Store build).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\demo\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "demo_camera.cpp"
#define T6_SOURCE "demo/demo_camera.cpp"

#include "t6_assert.h"
#include "t6_static.h"
#include "demo/demo.h"
#include "cgame/cg_local.h"

struct entityState_s;

// bgame/bg_animstatedef.cpp
bool BG_AnimStateDef_InUse();
bool BG_AnimStateDef_IsMissingLegs(const entityState_s *es);

// client_mp/cl_cgame_mp.cpp
void CL_ClampViewAngle(LocalClientNum_t localClientNum, int angle, float min, float max);

// 009034B0  demo/demo_camera.cpp:36 (cent in ESI)
// An AI's theater lock-on limits, from its AI type's script data; null for other entities and unset types.
AIDemoLockOnValues *Demo_GetAILockOnValues(centity_t *cent)
{
	assert_at(40, cent);
	unsigned char aiType;
	if (cent->eType == ET_SCRIPTMOVER)
		aiType = (unsigned char)cent->scriptMoverAiType;
	else if (cent->eType == ET_ACTOR || cent->eType == ET_ACTOR_CORPSE)
		aiType = (unsigned char)cent->actorAiType;
	else
		return 0;
	if (aiType && cg_level.aiTypes[aiType])
		return &cg_level.aiTypes[aiType]->demoLockOn;
	return 0;
}

// 00903510  demo/demo_camera.cpp:54 (cent in EAX)
// Which of the lock-on views applies: the second for an AI without legs. The Store's inlined copy always takes the
// first (its CL_CreateCmd 140194E20 reads views[0] with no test).
int Demo_GetAILockOnViewIndex(centity_t *cent)
{
	assert_at(57, cent);
#ifdef T6_STORE
	return 0;
#else
	return BG_AnimStateDef_InUse() && BG_AnimStateDef_IsMissingLegs((const entityState_s *)cent->nextState) ? 1 : 0;
#endif
}

// demo/demo_camera.cpp (inlined everywhere)
static demoCameraMode Demo_GetCameraMode()
{
	return Demo_IsPlaybackInited() ? demo.playback->cameraMode : DEMO_CAMERA_MODE_NONE;
}

// 009037C0  demo/demo_camera.cpp:448
bool Demo_IsThirdPersonCamera()
{
	return Demo_IsPlaying() && Demo_GetCameraMode() == DEMO_CAMERA_MODE_THIRDPERSON;
}

// 00903800  demo/demo_camera.cpp:463
bool Demo_IsAnyFreeMoveCamera()
{
	return Demo_IsPlaying() && Demo_GetCameraMode() == DEMO_CAMERA_MODE_FREECAM;
}

// 00906580  demo/demo_camera.cpp:532
bool Demo_IsFreeCameraLockedOnEntity()
{
	if (!Demo_IsPlaying() || !Demo_IsAnyFreeMoveCamera())
		return false;
	return Demo_IsPlaybackInited() && demo.playback->freeCameraLockedEntity != -1 &&
	       demo.playback->freeCameraLockedOnEntityActive;
}

// 009072F0  demo/demo_camera.cpp:1906
// The free camera, locked on to an entity, may only pitch within the entity's limits: 0 to 60 degrees, -60 to 60 for
// a helicopter, and an AI type's own for an AI. Nothing is clamped while the entity isn't valid.
void Demo_ClampViewAngleForLockedOnEntity(LocalClientNum_t localClientNum)
{
	assert_at(1913, Demo_IsFreeCameraLockedOnEntity());
	centity_t *cent = CG_GetEntity(localClientNum,
	                               Demo_IsPlaybackInited() ? demo.playback->freeCameraLockedEntity : -1);
	if (!(cent->packed_bits[0] & 2))
		return;
	float pitchMin = 0.0f;
	float pitchMax = 60.0f;
	if (cent->eType == ET_HELICOPTER)
	{
		pitchMin = -60.0f;
	}
	else if (cent->eType == ET_ACTOR)
	{
		AIDemoLockOnValues *lockOn = Demo_GetAILockOnValues(cent);
		if (lockOn)
		{
			int view = Demo_GetAILockOnViewIndex(cent);
			pitchMin = lockOn->views[view].pitchMin;
			pitchMax = lockOn->views[view].pitchMax;
		}
	}
	CL_ClampViewAngle(localClientNum, 0, pitchMin, pitchMax);
}
