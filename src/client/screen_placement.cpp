// client/screen_placement.cpp (CoDMPServer_PC: screen_placement.obj)
//
// Mapping the 640x480 virtual screen to the window. So far the scale the menu cursor uses.
#define T6_SOURCE "client/screen_placement.cpp"

#include "ui/ui.h"

#include "t6_static.h"

T6_GLOBAL ScreenPlacement scrPlaceFull;

// 00534070  client/screen_placement.cpp:79 (inlined in the Store build)
// No high-resolution scaling on PC.
float ScrPlace_HiResGetScaleX()
{
	return 1.0f;
}
