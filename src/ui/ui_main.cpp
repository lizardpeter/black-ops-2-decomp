// ui/ui_main.cpp (CoDMPServer_PC: ui_main.obj)
//
// The menu system's per-client state. So far what the mouse code reaches: the cursor's position in the 640x480
// virtual screen and handing mouse movement to the open menus. The Store build inlines all of these.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\ui\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "ui_main.cpp"
#define T6_SOURCE "ui/ui_main.cpp"

#include "ui/ui.h"

#include "t6_assert.h"
#include "t6_static.h"

void CL_ShowSystemCursor(int show);  // client/cl_input.cpp

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
