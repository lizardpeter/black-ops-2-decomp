// ui: the menu system (ui_main.cpp, ui_shared.cpp) and the screen placement it draws with.
#pragma once

#include "qcommon/qcommon.h"

#define MAX_UI_CONTEXTS 1  // a macro: the bounds asserts' text shows it expanded

// The 640x480 virtual screen's mapping to the window (0x7C bytes); only the fields reconstructed code reads.
struct ScreenPlacement
{
	float scaleVirtualToReal[2];
	float scaleVirtualToFull[2];
	unsigned char unknown10[0x7C - 0x10];
};
static_assert(sizeof(ScreenPlacement) == 0x7C, "ScreenPlacement");

extern ScreenPlacement scrPlaceFull;  // client/screen_placement.cpp

float ScrPlace_HiResGetScaleX();

// One UI context (0x21B4 bytes on the server); only the fields reconstructed code uses. Pointers in the menu table
// before menuCount make the x64 offsets larger (tools/pdb/layout64.py; the Store's IN_MouseMove agrees).
struct UiContext
{
	UIContextIndex_t contextIndex;
	float bias;
	int realTime;
	int frameTime;
	struct
	{
		float x;
		float y;
	} cursor;  // in the 640x480 virtual screen
	int isCursorVisible;
#if defined(_WIN64)
	unsigned char unknown1C[0x27B8 - 0x1C];
#else
	unsigned char unknown1C[0x14F4 - 0x1C];
#endif
	int menuCount;
#if defined(_WIN64)
	unsigned char unknown27BC[0x4130 - 0x27BC];
#else
	unsigned char unknown14F8[0x21B4 - 0x14F8];
#endif
};

enum uiMenuCommand_t
{
	UIMENU_NONE = 0,
	UIMENU_INGAME = 8,  // the in-game menu: keys go to the game (UI_KeysBypassMenu)
};

// The UI's state for one context (0x24E8 bytes on the server); only the fields reconstructed code uses.
struct uiInfo_s
{
	UiContext uiDC;
	int playerRefresh;
	int playerIndex;
	int timeIndex;
	int previousTimes[4];
	uiMenuCommand_t currentMenuType;
#if defined(_WIN64)
	unsigned char unknown4150[0x4468 - 0x4150];
#else
	unsigned char unknown21D4[0x24E8 - 0x21D4];
#endif
};
#if defined(_WIN64)
static_assert(__builtin_offsetof(uiInfo_s, uiDC.menuCount) == 0x27B8, "UiContext.menuCount (Store: IN_MouseMove 140499A10)");
static_assert(__builtin_offsetof(uiInfo_s, currentMenuType) == 0x414C, "uiInfo_s.currentMenuType (Store: IN_MouseMove 140499A10)");
#else
static_assert(sizeof(UiContext) == 0x21B4, "UiContext");
static_assert(sizeof(uiInfo_s) == 0x24E8, "uiInfo_s");
#endif

extern uiInfo_s uiInfoArray[MAX_UI_CONTEXTS];  // ui/ui_main.cpp

uiInfo_s *UI_GetInfo(LocalClientNum_t localClientNum);
bool UI_KeysBypassMenu(LocalClientNum_t localClientNum);
void UI_UpdateMouseScreenPosition(LocalClientNum_t localClientNum, int x, int y);
void UI_MouseEvent(LocalClientNum_t localClientNum, int x, int y);
int Display_MouseMove(LocalClientNum_t localClientNum, UiContext *dc);  // ui/ui_shared.cpp
