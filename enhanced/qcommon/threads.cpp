// enhanced/qcommon/threads.cpp: the render thread waits for work instead of spinning (P0008; B0024).
//
// A helper of enhanced/gfx_d3d/rb_backend.cpp's RB_RenderThread, here because the frame's event (backendEvent) is a
// static of threads.cpp. Store build only.
#ifdef T6_STORE
#define T6_SOURCE "qcommon/threads.cpp"

#include "qcommon/threads.h"
#include "t6_static.h"

#include <windows.h>

T6_STATIC HANDLE backendEvent[2] T6_AT("", "backendEvent");

// Blocks until a frame is handed over or one of the render thread's two other requests is set, or a millisecond has
// passed (the buffer resize and the remote screen updates are flags it still polls). Returns true when it took the
// frame's signal (backendEvent[1] resets as it is waited for): the caller draws that frame next.
bool Sys_RendererWaitForWork(HANDLE request1, HANDLE request2)
{
	HANDLE events[3] = {backendEvent[1], request1, request2};
	return WaitForMultipleObjects(3, events, FALSE, 1) == WAIT_OBJECT_0;
}
#endif
