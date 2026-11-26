#pragma once
#ifdef T6_STORE
struct dvar_t;
void UI_RegisterDvars();
// Shared full registration body. Faithful calls with true; an enhanced override may pass false.
void UI_RegisterDvarsWithBuildNumberDefault(bool drawBuildNumber);
#define STORE_UI_DVAR_GLOBALS(X) \
    X(ui_animSpeedScale) \
    X(ui_playlistPopulationRefreshTime) \
    X(ui_netSource) \
    X(ui_lastServerRefresh_0) \
    X(ui_lastServerRefresh_1) \
    X(ui_lastServerRefresh_2) \
    X(ui_lastServerRefresh_3) \
    X(ui_lastServerRefresh_4) \
    X(ui_smallFont) \
    X(ui_bigFont) \
    X(ui_language) \
    X(ui_languagechanged) \
    X(ui_currentMap) \
    X(ui_preview) \
    X(ui_mapPackChanged) \
    X(ui_prevTextEntryBox) \
    X(ui_ignoreMousePos) \
    X(ui_drawBuildNumber) \
    X(ui_demoname) \
    X(ui_menuLvlNotify) \
    X(selectedFriendName) \
    X(selectedFriendIndex) \
    X(selectedPlayerXuid) \
    X(selectedMenuItemIndex) \
    X(ui_showDLCMaps) \
    X(ui_scrollTextYOffset) \
    X(ui_scrollTextScrollRate) \
    X(ui_scrollTextYOffsetResetDelay) \
    X(ui_scrollTextDelay) \
    X(ui_scrollMOTDYOffset) \
    X(ui_MOTDScrollRate) \
    X(ui_scrollMOTDDelay) \
    X(ui_scrollMOTDYOffsetResetDelay) \
    X(ui_borderLowLightScale) \
    X(ui_lobbypopup) \
    X(ui_listboxIndex) \
    X(ls_status) \
    X(ui_keyboardtitle) \
    X(ui_keyboard_dvar_edit) \
    X(ls_demoauthor) \
    X(scr_hardcore) \
    X(ui_choice_noaction) \
    X(ui_hud_showobjicons) \
    X(ui_hud_obituaries) \
    X(tu7_ui_multiteam_compass_fix) \
    X(ui_fxFontColor) \
    X(ui_fxFontGlowColor) \
    X(ui_fxFontShadowColor) \
    X(ui_fxFontOutlineColor) \
    X(ui_drawOldUI) \
    X(tu_allowDLCWeaponsByOwnership) \
    X(ui_friendsListOpen) \
    X(ui_playerListOpen)
extern "C" {
#define STORE_UI_DVAR_DECLARE(name) extern const dvar_t *store_uiDvar_##name;
STORE_UI_DVAR_GLOBALS(STORE_UI_DVAR_DECLARE)
#undef STORE_UI_DVAR_DECLARE
}
#endif
