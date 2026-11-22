// demo/demo_playback.cpp (CoDMPServer_PC: demo_playback.obj)
//
// Demo playback. So far whether it is set up, which the demo camera checks read.
#define T6_SOURCE "demo/demo_playback.cpp"

#include "t6_static.h"
#include "demo/demo.h"

// 00910420  demo/demo_playback.cpp:867 (inlined in the Store build)
bool Demo_IsPlaybackInited()
{
	return demo.playbackInited;
}
