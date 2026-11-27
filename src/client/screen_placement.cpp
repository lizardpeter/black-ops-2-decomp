// client/screen_placement.cpp (CoDMPServer_PC: screen_placement.obj)
//
// Mapping the 640x480 virtual screen to the window. So far the scale the menu cursor uses.
#define T6_SOURCE "client/screen_placement.cpp"

#include "ui/ui.h"

#include "t6_static.h"

T6_GLOBAL ScreenPlacement scrPlaceFull;
T6_GLOBAL float cg_hudSplitscreenScale;
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
float ScrPlace_RealFromVirtual_Height(const ScreenPlacement *scrPlace, float height)
{
	return ScrPlace_HiResGetScaleY() * (cg_hudSplitscreenScale * height) * scrPlace->scaleVirtualToReal[1];
}
