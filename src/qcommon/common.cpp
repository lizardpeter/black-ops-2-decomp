// qcommon/common.cpp (CoDMPServer_PC: common.obj)
//
// The engine's common code. So far slow motion's scaling of the view's frame time.
#define T6_SOURCE "qcommon/common.cpp"

#include "t6_static.h"
#include "qcommon/qcommon.h"

T6_GLOBAL SlowMotionCommon g_slowmoCommon;

// 006BF470  qcommon/common.cpp:6125 (inlined in the Store build)
// A frame's time in seconds, scaled by the view's slow-motion timescale, which moves linearly from its start to its
// end value over the change's duration. The result is at least 0.001 (a NaN gives 0.001 too). The Store's /fp:fast
// copy tests the end differently when a timescale is NaN.
float Com_ViewScaleMsec(float sec)
{
	float msec = sec * 1000.0f;
	if (g_slowmoCommon.viewEnable)
	{
		float start = g_slowmoCommon.viewStartTimescale;
		float end = g_slowmoCommon.viewEndTimescale;
		bool done;
		if (g_slowmoCommon.viewEndMsec == g_slowmoCommon.viewStartMsec)
		{
			done = true;
		}
		else
		{
			g_slowmoCommon.viewTimescale += (end - start) * msec /
			                                (float)(g_slowmoCommon.viewEndMsec - g_slowmoCommon.viewStartMsec);
			float cur = g_slowmoCommon.viewTimescale;
#ifdef T6_STORE
			done = (!(start >= end) && cur >= end) || (start > end && !(cur > end)) || (!(start < end) && !(start > end));
#else
			done = (start < end && cur >= end) || (start > end && end >= cur) || start == end;
#endif
		}
		if (done)
		{
			g_slowmoCommon.viewTimescale = end;
			g_slowmoCommon.viewEnable = false;
		}
	}
	float scaled = g_slowmoCommon.viewTimescale * msec * 0.001f;
#ifdef T6_STORE
	return scaled >= 0.001f ? scaled : 0.001f;
#else
	return scaled - 0.001f >= 0.0f ? scaled : 0.001f;
#endif
}
