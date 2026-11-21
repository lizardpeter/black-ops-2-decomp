// cgame_mp/cg_main_mp.cpp (CoDMPServer_PC: cg_main_mp.obj)
//
// The multiplayer cgame's per-client state. So far the accessor the client's input code uses.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\cgame_mp\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "cg_main_mp.cpp"
#define T6_SOURCE "cgame_mp/cg_main_mp.cpp"

#include "cgame_mp/cg_local_mp.h"

// 00466D70  cgame_mp/cg_main_mp.cpp:981 (inlined everywhere in the Store build)
// Before the cgame starts there is no cg_t: the result is the field's offset from a null pointer, as in the original.
const playerState_s *CG_GetPredictedPlayerState(LocalClientNum_t localClientNum)
{
	return &CG_GetLocalClientGlobals(localClientNum)->predictedPlayerState;
}
