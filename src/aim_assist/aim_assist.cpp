// aim_assist/aim_assist.cpp (CoDMPServer_PC: aim_assist.obj)
//
// Aim assist. So far the mouse's entry point, which CL_MouseMove calls (and the Store build inlines there).
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\aim_assist\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "aim_assist.cpp"
#define T6_SOURCE "aim_assist/aim_assist.cpp"

#include "aim_assist/aim_assist.h"

#include "t6_assert.h"
#include "t6_static.h"

enum
{
	WEAPON_LOWREADY_LOOP = 0x38,  // weaponstate_t
	WEAPON_CRAWL_IN = 0x3A,
};

T6_GLOBAL AimAssistGlobals aaGlobArray[MAX_LOCAL_CLIENTS];

// 004096C0  aim_assist/aim_assist.cpp:2762
// The mouse gets no slowdown or auto-aim: the angles pass through, and only auto-melee may turn the player, except in
// weapon states 0x38-0x3A (low-ready loop and drop, crawl in).
void AimAssist_UpdateMouseInput(const AimInput *input, AimOutput *output)
{
	assert_at(2769, input);
	assert_at(2770, output);
	output->pitch = input->pitch;
	output->yaw = input->yaw;
	AimAssistGlobals *aaGlob = &aaGlobArray[input->localClientNum];
	if (!aaGlob->initialized)
		return;
	AimAssist_UpdateTweakables(input, input->localClientNum);
	AimAssist_UpdateAdsLerp(input);
	if (aaGlob->ps.weaponstate < WEAPON_LOWREADY_LOOP || aaGlob->ps.weaponstate > WEAPON_CRAWL_IN)
		AimAssist_ApplyAutoMelee(input, output);
	aaGlob->prev_button_bits = input->button_bits;
}
