// ui/ui_main.cpp (CoDMPServer_PC: ui_main.obj)
//
// The menu system's per-client state. So far what the mouse code reaches: the cursor's position in the 640x480
// virtual screen and handing mouse movement to the open menus. The Store build inlines all of these.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\ui\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "ui_main.cpp"
#define T6_SOURCE "ui/ui_main.cpp"

#include "ui/ui.h"

#include "universal/dvar.h"

#include "t6_assert.h"
#include "t6_static.h"

void CL_ShowSystemCursor(int show);  // client/cl_input.cpp

extern const dvar_t *ui_smallFont;     // registered by UI_RegisterDvars
extern const dvar_t *ui_bigFont;       // registered by UI_RegisterDvars
extern const dvar_t *ui_extraBigFont;  // registered with the hud's dvars

T6_GLOBAL sharedUiInfo_t sharedUiInfo;

// The Store build's compiler (/fp:fast, docs/store-build.md) tests the font scale's upper limits as !(scale > limit)
// (comiss, jbe), which only changes what a NaN does: it gets the small font.
#ifdef T6_STORE
#define FONT_SCALE_AT_MOST(scale, limit) (!((scale) > (limit)))
#else
#define FONT_SCALE_AT_MOST(scale, limit) ((scale) <= (limit))
#endif

T6_GLOBAL uiInfo_s uiInfoArray[MAX_UI_CONTEXTS];

// ui/ui_main.cpp:372 (inlined in UI_GetInfo)
static uiInfo_s *UI_GetInfoForContext(UIContextIndex_t contextIndex)
{
	assertf_at(373, (contextIndex == 0), "(contextIndex) = %i", contextIndex);
	return uiInfoArray;
}

// 0070E930  ui/ui_main.cpp:379
uiInfo_s *UI_GetInfo(LocalClientNum_t localClientNum)
{
	UIContextIndex_t contextIndex = Com_LocalClient_GetUIContextIndex(localClientNum);
	bcassert_at(381, contextIndex, MAX_UI_CONTEXTS);
	return UI_GetInfoForContext(contextIndex);
}

// 00713290  ui/ui_main.cpp:426
bool UI_KeysBypassMenu(LocalClientNum_t localClientNum)
{
	return UI_GetInfo(localClientNum)->currentMenuType == UIMENU_INGAME;
}

// 00711150  ui/ui_main.cpp:2389
// The cursor in the virtual screen; the system cursor shows while it is outside it.
void UI_UpdateMouseScreenPosition(LocalClientNum_t localClientNum, int x, int y)
{
	UiContext *dc = &UI_GetInfo(localClientNum)->uiDC;
	dc->cursor.x = (float)x / (ScrPlace_HiResGetScaleX() * scrPlaceFull.scaleVirtualToFull[0]);
	dc->cursor.y = (float)y / (ScrPlace_HiResGetScaleX() * scrPlaceFull.scaleVirtualToFull[1]);
	dc->isCursorVisible = dc->cursor.x >= 0.0f && dc->cursor.x <= 640.0f && dc->cursor.y >= 0.0f &&
	                      dc->cursor.y <= 480.0f;
	CL_ShowSystemCursor(!dc->isCursorVisible);
}

// 007111F0  ui/ui_main.cpp:2402
void UI_MouseEvent(LocalClientNum_t localClientNum, int x, int y)
{
	UiContext *dc = &UI_GetInfo(localClientNum)->uiDC;
	if (dc->isCursorVisible && dc->menuCount > 0)
		Display_MouseMove(localClientNum, dc);
}

// 00710500  ui/ui_main.cpp:1228
// Store MP 1403FCD20 (tests/replay/verify_ui_fonts.py)
// The font for a scale on the real screen: small up to ui_smallFont, extra big from ui_extraBigFont, big from
// ui_bigFont, otherwise the text font.
Font_s *UI_GetFontHandleDefault(float scale)
{
	if (ui_smallFont && FONT_SCALE_AT_MOST(scale, Dvar_GetFloat(ui_smallFont)))
		return sharedUiInfo.assets.smallFont;
	if (ui_extraBigFont && scale >= Dvar_GetFloat(ui_extraBigFont))
		return sharedUiInfo.assets.extraBigFont;
	if (ui_bigFont && scale >= Dvar_GetFloat(ui_bigFont))
		return sharedUiInfo.assets.bigFont;
	return sharedUiInfo.assets.textFont;
}

// 00710570  ui/ui_main.cpp:1240
// Store MP 1403FCE40 (tests/replay/verify_ui_fonts.py)
Font_s *UI_GetFontHandle(const ScreenPlacement *scrPlace, int fontEnum, float scale)
{
	switch (fontEnum)
	{
	case UI_FONT_NORMAL:
		return sharedUiInfo.assets.textFont;
	case UI_FONT_BIG:
		return sharedUiInfo.assets.bigFont;
	case UI_FONT_SMALL:
		return sharedUiInfo.assets.smallFont;
	case UI_FONT_CONSOLE:
		return sharedUiInfo.assets.consoleFont;
	case UI_FONT_EXTRABIG:
		return sharedUiInfo.assets.extraBigFont;
	case UI_FONT_EXTRASMALL:
		return sharedUiInfo.assets.extraSmallFont;
	case UI_FONT_FX:
		return sharedUiInfo.assets.fxFont;
	}
	const float realScale = ScrPlace_RealFromVirtual_Height(scrPlace, cg_hudSplitscreenScale * scale);
	if (fontEnum != UI_FONT_BOLD)
		return UI_GetFontHandleDefault(realScale);
	if (ui_smallFont && FONT_SCALE_AT_MOST(realScale, Dvar_GetFloat(ui_smallFont)))
		return sharedUiInfo.assets.smallFont;
	if (ui_bigFont && realScale >= Dvar_GetFloat(ui_bigFont))
		return sharedUiInfo.assets.boldFont;
	return sharedUiInfo.assets.textFont;
}
