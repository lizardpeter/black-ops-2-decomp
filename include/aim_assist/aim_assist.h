// aim_assist/aim_assist.cpp: aim assist, the input it is given and what it gives back.
#pragma once

#include "bgame/bg_public.h"
#include "qcommon/bitarray.h"
#include "qcommon/qcommon.h"

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
static_assert(sizeof(void *) != 8 || sizeof(AimInput) == 0x40, "AimInput (x64)");

struct AimOutput
{
	float pitch;
	float yaw;
	float meleeChargeYaw;
	unsigned short meleeChargeEnt;
	unsigned char meleeChargeDist;
};
static_assert(sizeof(AimOutput) == 0x10, "AimOutput");

// The player state aim assist keeps its own copy of (0x68 bytes); only the fields reconstructed code reads.
struct AimAssistPlayerState
{
	unsigned char unknown0[0x48];
	int weaponstate;  // weaponstate_t
	unsigned char unknown4C[0x68 - 0x4C];
};

// One local client's aim assist (0x12D0 bytes on the server); only the fields reconstructed code uses. The Store
// build has the same offsets (CL_MouseMove's inlined AimAssist_UpdateMouseInput).
struct AimAssistGlobals
{
	AimAssistPlayerState ps;
	LocalClientNum_t localClientNum;
	bool initialized;
	unsigned char unknown6D[0x12B0 - 0x6D];
	bitarray<64> prev_button_bits;
	unsigned char unknown12B8[0x12D0 - 0x12B8];
};
static_assert(__builtin_offsetof(AimAssistGlobals, initialized) == 0x6C, "AimAssistGlobals.initialized");
static_assert(__builtin_offsetof(AimAssistGlobals, prev_button_bits) == 0x12B0, "AimAssistGlobals.prev_button_bits");
static_assert(sizeof(AimAssistGlobals) == 0x12D0, "AimAssistGlobals");

extern AimAssistGlobals aaGlobArray[MAX_LOCAL_CLIENTS];

void AimAssist_UpdateTweakables(const AimInput *input, LocalClientNum_t localClientNum);
void AimAssist_UpdateAdsLerp(const AimInput *input);
void AimAssist_ApplyAutoMelee(const AimInput *input, AimOutput *output);
void AimAssist_UpdateMouseInput(const AimInput *input, AimOutput *output);
