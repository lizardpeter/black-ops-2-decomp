// ui: the menu system (ui_main.cpp, ui_shared.cpp) and the screen placement it draws with.
#pragma once

#include "qcommon/qcommon.h"

#define MAX_UI_CONTEXTS 1  // a macro: the bounds asserts' text shows it expanded

struct Font_s;
struct Material;

// The 640x480 virtual screen's mapping to the window (0x7C bytes, no pointers: the same in every build).
struct ScreenPlacement
{
	float scaleVirtualToReal[2];
	float scaleVirtualToFull[2];
	float scaleRealToVirtual[2];
	float virtualViewableMin[2];
	float virtualViewableMax[2];
	float virtualTweakableMin[2];
	float virtualTweakableMax[2];
	float realViewportBase[2];
	float realViewportSize[2];
	float realViewportMid[2];
	float realViewableMin[2];
	float realViewableMax[2];
	float realTweakableMin[2];
	float realTweakableMax[2];
	float subScreen[2];
	float hudSplitscreenScale;
};
static_assert(sizeof(ScreenPlacement) == 0x7C, "ScreenPlacement");

extern ScreenPlacement scrPlaceFull;  // client/screen_placement.cpp
extern float cg_hudSplitscreenScale;  // client/screen_placement.cpp

float ScrPlace_HiResGetScaleX();
float ScrPlace_HiResGetScaleY();
float ScrPlace_RealFromVirtual_Height(const ScreenPlacement *scrPlace, float height);

#ifdef T6_STORE
// The Store build's high-resolution screenshot state (CL_TakeHiResShot): while a shot is taken the screen is drawn
// scaled by scale[]. Unnamed in every build with symbols (the servers return 1 from ScrPlace_HiResGetScale*); the
// fields are what the Store's ScrPlace functions read.
struct HiResShot
{
	int active;
	float scale[2];
};
extern HiResShot s_hiResShot;  // client/screen_placement.cpp
#endif

// The menus' shared materials and fonts (CachedAssets_t). The Store build has no vertical scroll bar arrows: its
// UI_AssetCache registers its first material, "ui_scrollbar_arrow_left", at offset 0 and its fonts start at 0x68.
struct CachedAssets_t
{
#ifndef T6_STORE
	Material *scrollBarArrowUp;
	Material *scrollBarArrowDown;
#endif
	Material *scrollBarArrowLeft;
	Material *scrollBarArrowRight;
	Material *scrollBar;
	Material *scrollBarThumb;
	Material *sliderBar;
	Material *sliderThumb;
	Material *whiteMaterial;
	Material *cursor;
	Material *dvarlistArrowLeft;
	Material *dvarlistArrowRight;
	Material *line;
	Material *blur;
	Material *lineGraph;
	Font_s *bigFont;
	Font_s *smallFont;
	Font_s *consoleFont;
	Font_s *boldFont;
	Font_s *textFont;
	Font_s *extraBigFont;
	Font_s *extraSmallFont;
	Font_s *fxFont;
	void *itemFocusSound;
};
#if __SIZEOF_POINTER__ == 8 && defined(T6_STORE)
static_assert(__builtin_offsetof(CachedAssets_t, bigFont) == 0x68, "CachedAssets_t (Store)");
#elif __SIZEOF_POINTER__ == 4
static_assert(sizeof(CachedAssets_t) == 0x60, "CachedAssets_t");
#endif

// The menus' shared state; only its first member so far.
struct sharedUiInfo_t
{
	CachedAssets_t assets;
};
extern sharedUiInfo_t sharedUiInfo;  // ui/ui_main.cpp

enum
{
	UI_FONT_DEFAULT = 0,
	UI_FONT_NORMAL = 1,
	UI_FONT_BIG = 2,
	UI_FONT_SMALL = 3,
	UI_FONT_BOLD = 4,
	UI_FONT_CONSOLE = 5,
	UI_FONT_EXTRABIG = 6,
	UI_FONT_EXTRASMALL = 7,
	UI_FONT_FX = 8,
};

Font_s *UI_GetFontHandleDefault(float scale);
Font_s *UI_GetFontHandle(const ScreenPlacement *scrPlace, int fontEnum, float scale);

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
