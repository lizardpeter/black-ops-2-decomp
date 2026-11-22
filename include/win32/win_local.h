// win32/win_local.h: the PC platform layer's shared state.
#pragma once

#include <windows.h>

struct WinVars_t
{
	HINSTANCE reflib_library;
	int reflib_active;
	HWND hWnd;
	HINSTANCE hInstance;
	int activeApp;
	int isMinimized;
	int recenterMouse;     // the last CL_MouseEvent wanted the cursor centred (gameplay, not a menu)
	HHOOK lowLevelKeyboardHook;
	unsigned int sysMsgTime;
};
static_assert(sizeof(void *) != 4 || sizeof(WinVars_t) == 0x24, "WinVars_t (the server's x86 layout)");

extern WinVars_t g_wv;  // win_wndproc.cpp

// win_input.cpp's mouse state (a file static there)
struct WinMouseVars_t
{
	int oldButtonState;
	POINT oldPos;          // the cursor's screen position at the last read
	bool mouseActive;      // the game window has the mouse
	bool mouseInitialized;
};

// win_input.cpp
void IN_Frame();
void IN_Shutdown();
void IN_ActivateMouse(int force);
void IN_DeactivateWin32Mouse();
void IN_RecenterMouse();
void IN_MouseMove();
void IN_GamepadsMove();
void IN_SetCursorPos(int x, int y);
void IN_ShowSystemCursor(int show);
void IN_ClampMouseMove(POINT *curPos);
