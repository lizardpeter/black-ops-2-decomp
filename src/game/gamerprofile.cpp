// game/gamerprofile.cpp (CoDMPServer_PC: gamerprofile.obj)
//
// The player's profile settings. So far the float getter the mouse code uses.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\game\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "gamerprofile.cpp"
#define T6_SOURCE "game/gamerprofile.cpp"

#include "game/gamerprofile.h"

// 00675010  game/gamerprofile.cpp:536 (inlined in the Store build, whose callers call GamerProfile_GetInt and use its
// bits as the float)
// A float setting is stored as its bits in the int.
float GamerProfile_GetFloat(profile_setting setting, ControllerIndex_t controllerIndex)
{
	int value = GamerProfile_GetInt(setting, controllerIndex);
	return *(float *)&value;
}
