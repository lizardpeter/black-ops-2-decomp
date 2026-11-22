// enhanced/win32/win_input.cpp: the mouse turns the view by raw device counts (P0002; B0015).
//
// IN_MouseMove still reads the cursor for menus, which keep their absolute cursor, and still recentres it while the
// game has the mouse, so clicks stay in the window. What the game turns by is the raw input thread's device counts
// since the last frame, passed to CL_MouseEvent in place of the cursor's movement. Store build only.
#ifdef T6_STORE
#define T6_SOURCE "win32/win_input.cpp"

#include "enhanced/raw_input.h"

#include "client/client.h"
#include "qcommon/qcommon.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_local.h"

int CL_MouseEvent(LocalClientNum_t localClientNum, int x, int y, int dx, int dy);
extern const dvar_t *r_fullscreen;

T6_STATIC WinMouseVars_t s_wmv T6_AT("", "s_wmv");
T6_STATIC int window_center_x T6_AT("", "window_center_x");
T6_STATIC int window_center_y T6_AT("", "window_center_y");

void IN_MouseMove()
{
	IN_StartRawInput();
	int rawDx, rawDy;
	IN_TakeRawMouse(&rawDx, &rawDy);  // always: counts never wait for a later frame
	if (GetForegroundWindow() != g_wv.hWnd)
		return;
	POINT curPos;
	GetCursorPos(&curPos);
	if (Dvar_GetInt(r_fullscreen) == 1)
		IN_ClampMouseMove(&curPos);
	int dx = curPos.x - s_wmv.oldPos.x;
	int dy = curPos.y - s_wmv.oldPos.y;
	if (dx || dy || rawDx || rawDy)
		Com_LocalClient_LastInput_Set(Com_LocalClients_GetPrimary(), LAST_INPUT_MOUSEMOVE);
	s_wmv.oldPos.x = curPos.x;
	s_wmv.oldPos.y = curPos.y;
	ScreenToClient(g_wv.hWnd, &curPos);
	// menus use the cursor's position (and only whether the mouse moved); the game adds the counts to mouseIn
	g_wv.recenterMouse = CL_MouseEvent(LOCAL_CLIENT_0, curPos.x, curPos.y, rawDx, rawDy);
	if (g_wv.recenterMouse && (dx || dy))
	{
		IN_RecenterMouse();
		s_wmv.oldPos.x = window_center_x;
		s_wmv.oldPos.y = window_center_y;
	}
}
#endif
