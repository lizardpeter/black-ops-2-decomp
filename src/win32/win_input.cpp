// win32/win_input.cpp (CoDMPServer_PC: win_input.obj)
//
// PC mouse and gamepad polling, once a frame from CL_Input. The mouse is the Windows cursor: IN_MouseMove reads how
// far it moved since the last frame and puts it back in the middle of the window, so Windows' pointer settings
// (acceleration, "Enhance pointer precision") apply to it and the screen's edges can cut a fast movement short
// (B0015). Menus see the cursor's position in the window.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\win32\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "win_input.cpp"
#define T6_SOURCE "win32/win_input.cpp"

#include "win32/win_local.h"

#include <windows.h>

#include "client/client.h"
#include "qcommon/qcommon.h"
#include "t6_assert.h"
#include "t6_static.h"
#include "universal/dvar.h"
#include "win32/win_shared.h"

struct WinMouseVars_t
{
	int oldButtonState;
	POINT oldPos;          // the cursor's screen position at the last read
	bool mouseActive;      // the game window has the mouse
	bool mouseInitialized;
};

enum GamePadStick
{
	GPAD_INVALID = 0,
	GPAD_LX = 0x40000000,
	GPAD_LY = 0x40000001,
	GPAD_RX = 0x40000002,
	GPAD_RY = 0x40000003,
};

enum GamePadButton
{
	GPAD_NONE = 0,
	GPAD_UP = 0x10000001,
	GPAD_DOWN = 0x10000002,
	GPAD_LEFT = 0x10000004,
	GPAD_RIGHT = 0x10000008,
	GPAD_START = 0x10000010,
	GPAD_BACK = 0x10000020,
	GPAD_L3 = 0x10000040,
	GPAD_R3 = 0x10000080,
	GPAD_L_SHLDR = 0x10000100,
	GPAD_R_SHLDR = 0x10000200,
	GPAD_A = 0x10001000,
	GPAD_B = 0x10002000,
	GPAD_X = 0x10004000,
	GPAD_Y = 0x10008000,
	GPAD_L_TRIG = 0x20000000,
	GPAD_R_TRIG = 0x20000001,
};

struct ButtonToCodeMap_t
{
	GamePadButton padButton;
	int code;  // the key code the button sends
};

enum LastInput_t
{
	LAST_INPUT_GAMEPAD = 0,
	LAST_INPUT_MOUSEMOVE = 1,
	LAST_INPUT_MOUSECLICK = 2,
	LAST_INPUT_KEYBOARD = 3,
};

// win_gamepad.cpp
void GPad_UpdateAll();
bool GPad_IsActive(ControllerIndex_t portIndex);
float GPad_GetStick(ControllerIndex_t portIndex, GamePadStick stick);
float GPad_GetButton(ControllerIndex_t portIndex, GamePadButton button);
bool GPad_IsButtonPressed(ControllerIndex_t portIndex, GamePadButton button);
bool GPad_ButtonRequiresUpdates(ControllerIndex_t portIndex, GamePadButton button);
bool GPad_IsButtonReleased(ControllerIndex_t portIndex, GamePadButton button);

// client
void CL_GamepadEvent(ControllerIndex_t portIndex, int gamePadEvent, int value, int time);
void CL_GamepadButtonEventForPort(ControllerIndex_t portIndex, int key, int buttonEvent, unsigned int time,
                                  int button);
int CL_MouseEvent(LocalClientNum_t localClientNum, int x, int y, int dx, int dy);

// qcommon/com_clients.cpp
LocalClientNum_t Com_LocalClients_GetPrimary();
void Com_LocalClient_LastInput_Set(LocalClientNum_t localClientNum, LastInput_t lastInput);

extern const dvar_t *r_fullscreen;     // gfx_d3d/r_dvars.cpp
extern const dvar_t *ClickToContinue;  // qcommon/common.cpp

T6_GLOBAL const dvar_t *in_mouse;
T6_GLOBAL int g_showCursor;
T6_GLOBAL ButtonToCodeMap_t buttonList[16];

T6_STATIC int in_appactive T6_AT("", "in_appactive");
T6_STATIC WinMouseVars_t s_wmv T6_AT("", "s_wmv");
T6_STATIC int window_center_x T6_AT("", "window_center_x");
T6_STATIC int window_center_y T6_AT("", "window_center_y");

// 00797A70  win32/win_input.cpp:121
// The first controller: sticks and triggers as analog events (-65535 to 65535; triggers 0 to 65535), then each
// mapped button's press, hold or release.
void IN_GamepadsMove()
{
	GPad_UpdateAll();
	int time = Sys_Milliseconds();
	if (!GPad_IsActive(CONTROLLER_INDEX_0))
		return;
	// the x87 product truncated unrounded: in double, the same
	int lx = (int)((double)GPad_GetStick(CONTROLLER_INDEX_0, GPAD_LX) * 65535.0f);
	int ly = (int)((double)GPad_GetStick(CONTROLLER_INDEX_0, GPAD_LY) * 65535.0f);
	int rx = (int)((double)GPad_GetStick(CONTROLLER_INDEX_0, GPAD_RX) * 65535.0f);
	int ry = (int)((double)GPad_GetStick(CONTROLLER_INDEX_0, GPAD_RY) * 65535.0f);
	int leftTrig = (int)((double)GPad_GetButton(CONTROLLER_INDEX_0, GPAD_L_TRIG) * 65535.0f);
	int rightTrig = (int)((double)GPad_GetButton(CONTROLLER_INDEX_0, GPAD_R_TRIG) * 65535.0f);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 0, rx, time);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 1, ry, time);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 2, lx, time);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 3, ly, time);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 5, leftTrig, time);
	CL_GamepadEvent(CONTROLLER_INDEX_0, 4, rightTrig, time);
	bool buttonPressed = false;
	for (unsigned int i = 0; i < sizeof(buttonList) / sizeof(buttonList[0]); ++i)
	{
		if (GPad_IsButtonPressed(CONTROLLER_INDEX_0, buttonList[i].padButton))
		{
			CL_GamepadButtonEventForPort(CONTROLLER_INDEX_0, buttonList[i].code, 1, time, buttonList[i].padButton);
			buttonPressed = true;
		}
		else if (GPad_ButtonRequiresUpdates(CONTROLLER_INDEX_0, buttonList[i].padButton))
		{
			CL_GamepadButtonEventForPort(CONTROLLER_INDEX_0, buttonList[i].code, 2, time, buttonList[i].padButton);
		}
		else if (GPad_IsButtonReleased(CONTROLLER_INDEX_0, buttonList[i].padButton))
		{
			CL_GamepadButtonEventForPort(CONTROLLER_INDEX_0, buttonList[i].code, 0, time, buttonList[i].padButton);
		}
	}
	if (buttonPressed || rightTrig || leftTrig || ry || rx || ly || lx)
		Com_LocalClient_LastInput_Set(Com_ControllerIndex_GetLocalClientNum(CONTROLLER_INDEX_0), LAST_INPUT_GAMEPAD);
}

// 00797C70  win32/win_input.cpp:219
void IN_RecenterMouse()
{
	RECT window_rect;
	GetWindowRect(g_wv.hWnd, &window_rect);
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;
	SetCursorPos(window_center_x, window_center_y);
}

// 00797F80  win32/win_input.cpp:235
// Shows the cursor: ShowCursor keeps a count, stepped until it is 0.
void IN_DeactivateWin32Mouse()
{
	g_showCursor = 1;
	for (int count = ShowCursor(TRUE); count != 0; count = ShowCursor(count < 0))
		;
}

// 00797CD0  win32/win_input.cpp:330
void IN_ActivateMouse(int force)
{
	if (!s_wmv.mouseInitialized)
		return;
	assert_at(333, r_fullscreen);
	if (!Dvar_GetBool(in_mouse))
	{
		s_wmv.mouseActive = false;
		return;
	}
	if (force || !s_wmv.mouseActive)
		s_wmv.mouseActive = GetForegroundWindow() == g_wv.hWnd;
}

// 00797D50  win32/win_input.cpp:430
// Keeps the cursor inside the window (fullscreen).
void IN_ClampMouseMove(POINT *curPos)
{
	RECT rc;
	GetWindowRect(g_wv.hWnd, &rc);
	bool clamped = false;
	if (curPos->x < rc.left)
	{
		curPos->x = rc.left;
		clamped = true;
	}
	else if (curPos->x >= rc.right)
	{
		curPos->x = rc.right - 1;
		clamped = true;
	}
	if (curPos->y < rc.top)
	{
		curPos->y = rc.top;
		clamped = true;
	}
	else if (curPos->y >= rc.bottom)
	{
		curPos->y = rc.bottom - 1;
		clamped = true;
	}
	if (clamped)
		SetCursorPos(curPos->x, curPos->y);
}

// 00797DC0  win32/win_input.cpp:470
// The cursor's movement since the last read, to CL_MouseEvent; while the game wants it (not a menu), the cursor goes
// back to the window's centre.
void IN_MouseMove()
{
	assert_at(475, s_wmv.mouseInitialized);
	assert_at(476, r_fullscreen);
	if (GetForegroundWindow() != g_wv.hWnd)
		return;
	POINT curPos;
	GetCursorPos(&curPos);
	if (Dvar_GetInt(r_fullscreen) == 1)
		IN_ClampMouseMove(&curPos);
	int dx = curPos.x - s_wmv.oldPos.x;
	int dy = curPos.y - s_wmv.oldPos.y;
	if (dx || dy)
		Com_LocalClient_LastInput_Set(Com_LocalClients_GetPrimary(), LAST_INPUT_MOUSEMOVE);
	s_wmv.oldPos.x = curPos.x;
	s_wmv.oldPos.y = curPos.y;
	ScreenToClient(g_wv.hWnd, &curPos);
	g_wv.recenterMouse = CL_MouseEvent(LOCAL_CLIENT_0, curPos.x, curPos.y, dx, dy);
	if (g_wv.recenterMouse && (dx || dy))
	{
		IN_RecenterMouse();
		s_wmv.oldPos.x = window_center_x;
		s_wmv.oldPos.y = window_center_y;
	}
}

// 00797EF0  win32/win_input.cpp:506
// Moves the cursor to a point in the window (menus).
void IN_SetCursorPos(int x, int y)
{
	POINT curPos;
	curPos.x = x;
	curPos.y = y;
	ClientToScreen(g_wv.hWnd, &curPos);
	SetCursorPos(curPos.x, curPos.y);
	s_wmv.oldPos.x = curPos.x;
	s_wmv.oldPos.y = curPos.y;
}

// 00797F40  win32/win_input.cpp:522
// Steps ShowCursor's count to 0 (shown) or -1 (hidden).
void IN_ShowSystemCursor(int show)
{
	g_showCursor = show;
	int target = show ? 0 : -1;
	for (int count = ShowCursor(show); count != target; count = ShowCursor(count < target))
		;
}

// Inlined (IN_Shutdown, IN_Frame): gives the mouse back to Windows.
static void IN_DeactivateMouse()
{
	if (s_wmv.mouseInitialized && s_wmv.mouseActive)
	{
		s_wmv.mouseActive = false;
		IN_DeactivateWin32Mouse();
	}
}

// 00797FB0  win32/win_input.cpp:568
void IN_Shutdown()
{
	IN_DeactivateMouse();
}

// 00797FD0  win32/win_input.cpp:615
void IN_Frame()
{
	if (Dvar_GetBool(ClickToContinue))
		PostMessageA(g_wv.hWnd, WM_LBUTTONDOWN, MK_LBUTTON, 0);  // a click every frame
	if (!s_wmv.mouseInitialized)
		return;
	if (!in_appactive)
	{
		IN_DeactivateMouse();
		return;
	}
	IN_ActivateMouse(0);
	IN_MouseMove();
	if (GetForegroundWindow() == g_wv.hWnd)
		IN_GamepadsMove();
}
