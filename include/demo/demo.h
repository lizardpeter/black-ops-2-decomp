// demo: demo recording and playback (demo_common.cpp and the files around it).
#pragma once

#include "qcommon/qcommon.h"

enum demoState
{
	DEMO_STATE_IDLE = 0,
	DEMO_STATE_RECORDING = 1,
	DEMO_STATE_PLAYING = 2,
	DEMO_STATE_COUNT = 3,
};

enum demoCameraMode
{
	DEMO_CAMERA_MODE_NONE = 0,
	DEMO_CAMERA_MODE_THIRDPERSON = 1,
	DEMO_CAMERA_MODE_FREECAM = 2,
	DEMO_CAMERA_MODE_COUNT = 3,
};

// Demo playback's state (0x26A980 bytes on the server); only the fields reconstructed code reads. The Store's
// revision has the free camera's lock 0x80 bytes further on (its CL_CreateCmd 140194E20).
struct demoPlayback
{
	unsigned char unknown0[0xD4C00];
	demoCameraMode cameraMode;
#if defined(_WIN64)
	unsigned char unknownD4C04[0xEBD34 - 0xD4C04];
#else
	unsigned char unknownD4C04[0xEBCB4 - 0xD4C04];
#endif
	int freeCameraLockedEntity;
	bool freeCameraLockedOnEntityActive;
};

// The demo system's state (0xB0700 bytes in the server's PDB); only the fields reconstructed code reads. The Store's
// revision has four more bytes before state (its demo is at 1425A3280: state at +0x88, playback at +0xB0680).
struct demoMain
{
	char demoName[128];
	int demoFileHandle;
#if defined(_WIN64)
	int unknown84;
#endif
	demoState state;
#if defined(_WIN64)
	unsigned char unknown8C[0xB0680 - 0x8C];
#else
	unsigned char unknown88[0xB0680 - 0x88];
#endif
	demoPlayback *playback;
	bool playbackInited;
};
#if defined(_WIN64)
static_assert(__builtin_offsetof(demoMain, state) == 0x88, "demoMain.state (Store)");
static_assert(__builtin_offsetof(demoMain, playbackInited) == 0xB0688, "demoMain.playbackInited (Store)");
static_assert(__builtin_offsetof(demoPlayback, freeCameraLockedOnEntityActive) == 0xEBD38, "demoPlayback (Store)");
#else
static_assert(__builtin_offsetof(demoMain, playbackInited) == 0xB0684, "demoMain.playbackInited");
static_assert(__builtin_offsetof(demoPlayback, freeCameraLockedOnEntityActive) == 0xEBCB8, "demoPlayback");
#endif

extern demoMain demo;  // demo_common.cpp

bool Demo_IsPlaying();
bool Demo_IsPlaybackInited();
bool Demo_IsThirdPersonCamera();
bool Demo_IsAnyFreeMoveCamera();
bool Demo_IsFreeCameraLockedOnEntity();
void Demo_ClampViewAngleForLockedOnEntity(LocalClientNum_t localClientNum);
