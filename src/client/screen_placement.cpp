// client/screen_placement.cpp (CoDMPServer_PC: screen_placement.obj)
//
// Mapping the 640x480 virtual screen to the window: the placements, their conversions and ScrPlace_ApplyRect, which
// every 2D draw goes through. The Store build adds the high-resolution screenshot's scale and tile offset.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\client\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "screen_placement.cpp"
#define T6_SOURCE "client/screen_placement.cpp"

#include "ui/ui.h"

#include "client/client.h"
#include "t6_assert.h"
#include "t6_compiler.h"
#include "universal/dvar.h"

#include "t6_static.h"

T6_GLOBAL ScreenPlacement scrPlaceFull;
T6_GLOBAL ScreenPlacement scrPlaceView[MAX_UI_CONTEXTS];
T6_GLOBAL float cg_hudSplitscreenScale;
#ifndef T6_STORE
extern const dvar_t *cg_hudSplitscreenOffsetsUseScale;  // cgame
#endif
#ifdef T6_STORE
T6_GLOBAL HiResShot s_hiResShot;
#endif

// 00534070  client/screen_placement.cpp:79 (inlined in the Store build)
// The scale a high-resolution screenshot draws at: none on the servers (X and Y are one function there); in the Store
// build, s_hiResShot's while a shot is taken.
float ScrPlace_HiResGetScaleX()
{
#ifdef T6_STORE
	return s_hiResShot.active ? s_hiResShot.scale[0] : 1.0f;
#else
	return 1.0f;
#endif
}

// 00534070  client/screen_placement.cpp:79 (inlined in the Store build)
float ScrPlace_HiResGetScaleY()
{
#ifdef T6_STORE
	return s_hiResShot.active ? s_hiResShot.scale[1] : 1.0f;
#else
	return 1.0f;
#endif
}

// 00404500  client/screen_placement.h:111 (inlined everywhere)
X87_MATH float ScrPlace_RealFromVirtual_Height(const ScreenPlacement *scrPlace, float height)
{
	return ScrPlace_HiResGetScaleY() * (cg_hudSplitscreenScale * height) * scrPlace->scaleVirtualToReal[1];
}

// 00533F10  client/screen_placement.cpp:38 (inlined in the Store build)
const ScreenPlacement *ScrPlace_GetView(LocalClientNum_t localClientNum)
{
	bcassert_at(40, localClientNum, MAX_LOCAL_CLIENTS);
	const UIContextIndex_t contextIndex = Com_LocalClient_GetUIContextIndex(localClientNum);
	bcassert_at(42, contextIndex, MAX_UI_CONTEXTS);
	return &scrPlaceView[contextIndex];
}

// 004044E0  client/screen_placement.h:100 (inlined everywhere)
X87_MATH float ScrPlace_RealFromVirtual_Width(const ScreenPlacement *scrPlace, float width)
{
	return ScrPlace_HiResGetScaleX() * (cg_hudSplitscreenScale * width) * scrPlace->scaleVirtualToReal[0];
}

// client/screen_placement.h (Store MP 1400DDB40, tests/replay/verify_screen_placement.py; only the height has a
// symbol, 0043A230)
X87_MATH float ScrPlace_VirtualFromReal_Width(const ScreenPlacement *scrPlace, float width)
{
	return width / cg_hudSplitscreenScale / ScrPlace_HiResGetScaleX() * scrPlace->scaleRealToVirtual[0];
}

// 0043A230  client/screen_placement.h:130
// Store MP 1400D7110 (tests/replay/verify_screen_placement.py)
X87_MATH float ScrPlace_VirtualFromReal_Height(const ScreenPlacement *scrPlace, float height)
{
	return height / cg_hudSplitscreenScale / ScrPlace_HiResGetScaleY() * scrPlace->scaleRealToVirtual[1];
}

// 00534EE0  client/screen_placement.cpp:561 (inlined in the Store build)
static void ApplySplitscreenScaling(float *x, float *y, float *w, float *h, float hudSplitscreenScale)
{
	assert_at(562, x);
	assert_at(563, y);
	assert_at(564, w);
	assert_at(565, h);
#ifdef T6_STORE
	// no cg_hudSplitscreenOffsetsUseScale in the Store build: the position always scales
	*x = hudSplitscreenScale * *x;
	*y = hudSplitscreenScale * *y;
#else
	if (Dvar_GetBool(cg_hudSplitscreenOffsetsUseScale))
	{
		*x = *x * hudSplitscreenScale;
		*y = *y * hudSplitscreenScale;
	}
#endif
	*w = hudSplitscreenScale * *w;
	*h = hudSplitscreenScale * *h;
}

// 008B3F40  client/screen_placement.cpp:187 (empty on the servers; inlined in the Store build)
// While a high-resolution screenshot is taken, the screen is drawn scaled by s_hiResShot.scale and moved to the tile
// at s_hiResShot.offset (in screens).
void ScrPlace_HiResApplyRect(float *x, float *y, float *w, float *h)
{
#ifdef T6_STORE
	if (!s_hiResShot.active)
		return;
	*x = s_hiResShot.scale[0] * *x;
	*y = s_hiResShot.scale[1] * *y;
	*w = s_hiResShot.scale[0] * *w;
	*h = s_hiResShot.scale[1] * *h;
	*x = *x - (float)cls.vidConfig.displayWidth * s_hiResShot.offset[0] * s_hiResShot.scale[0];
	*y = *y - (float)cls.vidConfig.displayHeight * s_hiResShot.offset[1] * s_hiResShot.scale[1];
#endif
}

// 00534FD0  client/screen_placement.cpp:579
// Store MP 1401F65E0 (tests/replay/verify_screen_placement.py)
// A rectangle from virtual coordinates to the real screen: scaled for split screen, then placed per axis by its
// alignment (0 sub-screen, 1/3 viewable left/right, 2 viewport center, 4 full screen, 5 none, 6 real to virtual,
// 7 viewable center, 8/10 tweakable left/right, 9 tweakable center).
void ScrPlace_ApplyRect(const ScreenPlacement *scrPlace, float *x, float *y, float *w, float *h, int horzAlign,
                        int vertAlign)
{
	ApplySplitscreenScaling(x, y, w, h, scrPlace->hudSplitscreenScale);
	switch (horzAlign)
	{
	case 1:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->realViewableMin[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 2:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->realViewportSize[0] * 0.5f + scrPlace->realViewportBase[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 3:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->realViewableMax[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 4:
		*x = (scrPlace->realViewportBase[0] + scrPlace->scaleVirtualToFull[0]) * *x;
		*w = scrPlace->scaleVirtualToFull[0] * *w;
		break;
	case 5:
		break;
	case 6:
		*x = *x * scrPlace->scaleRealToVirtual[0];
		*w = *w * scrPlace->scaleRealToVirtual[0];
		break;
	case 7:
		*x = (scrPlace->realViewableMax[0] + scrPlace->realViewableMin[0]) * 0.5f + scrPlace->scaleVirtualToReal[0] * *x;
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 8:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->realTweakableMin[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 9:
		*x = (scrPlace->realTweakableMax[0] + scrPlace->realTweakableMin[0]) * 0.5f + scrPlace->scaleVirtualToReal[0] * *x;
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	case 10:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->realTweakableMax[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	default:
		assertfail_at(599, "invalid horizontal alignment case");
		// fall through: the sub-screen
	case 0:
		*x = scrPlace->scaleVirtualToReal[0] * *x + scrPlace->subScreen[0];
		*w = scrPlace->scaleVirtualToReal[0] * *w;
		break;
	}
	switch (vertAlign)
	{
	case 1:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->realViewableMin[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 2:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->realViewportSize[1] * 0.5f + scrPlace->realViewportBase[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 3:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->realViewableMax[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 4:
		*y = (scrPlace->realViewportBase[1] + scrPlace->scaleVirtualToFull[1]) * *y;
		*h = scrPlace->scaleVirtualToFull[1] * *h;
		break;
	case 5:
		break;
	case 6:
		*y = *y * scrPlace->scaleRealToVirtual[1];
		*h = *h * scrPlace->scaleRealToVirtual[1];
		break;
	case 7:
		*y = (scrPlace->realViewableMax[1] + scrPlace->realViewableMin[1]) * 0.5f + *y * scrPlace->scaleVirtualToReal[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 8:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->realTweakableMin[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 9:
		*y = (scrPlace->realTweakableMax[1] + scrPlace->realTweakableMin[1]) * 0.5f + *y * scrPlace->scaleVirtualToReal[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	case 10:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->realTweakableMax[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	default:
		assertfail_at(647, "invalid vertical alignment case");
		// fall through: the sub-screen
	case 0:
		*y = *y * scrPlace->scaleVirtualToReal[1] + scrPlace->subScreen[1];
		*h = *h * scrPlace->scaleVirtualToReal[1];
		break;
	}
	ScrPlace_HiResApplyRect(x, y, w, h);
}
