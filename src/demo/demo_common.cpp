// demo/demo_common.cpp (CoDMPServer_PC: demo_common.obj)
//
// Demo recording and playback. So far the state query the client's input code uses.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\demo\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "demo_common.cpp"
#define T6_SOURCE "demo/demo_common.cpp"

#include "t6_static.h"
#include "demo/demo.h"

T6_GLOBAL demoMain demo;

// 00909430  demo/demo_common.cpp:2375 (inlined in the Store build)
bool Demo_IsPlaying()
{
	return demo.state == DEMO_STATE_PLAYING;
}
