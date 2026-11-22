// client_mp/cl_cgame_mp.cpp (CoDMPServer_PC: cl_cgame_mp.obj)
//
// The client's side of the cgame interface. So far the view angle clamp.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\client_mp\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "cl_cgame_mp.cpp"
#define T6_SOURCE "client_mp/cl_cgame_mp.cpp"

#include "t6_assert.h"
#include "client/client.h"

// 004F1250  client_mp/cl_cgame_mp.cpp:1266 (inlined in the Store build)
// Clamps one view angle into [min, max] (the original's fsel form: a NaN angle stays NaN).
void CL_ClampViewAngle(LocalClientNum_t localClientNum, int angle, float min, float max)
{
	clientActive_t *cl;
	assert_at(1269, min <= max);
	bcassert_at(1270, angle, ARRAY_COUNT(cl->viewangles));
	cl = CL_GetLocalClientGlobals(localClientNum);
	float value = cl->viewangles[angle];
	float clamped = value - max >= 0.0f ? max : value;
	cl->viewangles[angle] = min - value >= 0.0f ? min : clamped;
}
