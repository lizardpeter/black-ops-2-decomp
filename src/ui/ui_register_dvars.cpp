// Store MP 1403F9B20 / ZM 1403FA4F0. Full UI registration body.
#define T6_SOURCE "ui/ui_main.cpp"
#ifdef T6_STORE
#include "ui/ui_register_dvars.h"
// Exact observed Store call boundaries; names prefixed Store_UI are provisional.
extern const dvar_t *_Dvar_RegisterFloat(const char *, float, float, float, unsigned, const char *);
extern const dvar_t *_Dvar_RegisterBool(const char *, bool, unsigned, const char *);
union alignas(16) StoreUiDvarValue { unsigned words[4]; const char *string; };
struct alignas(16) StoreUiDvarDomain { unsigned words[4]; };
using Value = StoreUiDvarValue;
using Domain = StoreUiDvarDomain;
extern "C" {
const dvar_t *Store_UI_FindDvar(const char *name);
const dvar_t *Store_UI_RegisterNew(const char *name, unsigned type, unsigned flags, const Value *value, const Domain *domain, const char *description);
void Store_UI_Reregister(const dvar_t *dvar, const char *name, unsigned type, unsigned flags, const Value *value, const Domain *domain, const char *description);
const dvar_t *Store_UI_RegisterString(const char *name, const char *value, unsigned flags);
const dvar_t *Store_UI_RegisterColor(const char *name, float r, float g, float b, float a, unsigned flags);
void Store_UI_RegisterRelatedDvars();
void Store_UI_RegisterTailDvars();
}
namespace {
const dvar_t *Register(const char *name, unsigned type, unsigned flags, const Value &value, const Domain &domain) {
    if (const auto *dvar = Store_UI_FindDvar(name)) {
        Store_UI_Reregister(dvar, name, type, flags, &value, &domain, "");
        return dvar;
    }
    return Store_UI_RegisterNew(name, type, flags, &value, &domain, "");
}
const dvar_t *RegisterBool(const char *name, bool value, unsigned flags) {
    return Register(name, 1, flags, Value{{value ? 1u : 0u}}, Domain{});
}
const dvar_t *RegisterInt(const char *name, unsigned value, unsigned min, unsigned max, unsigned flags) {
    return Register(name, 6, flags, Value{{value}}, Domain{{min, max}});
}
const dvar_t *RegisterString(const char *name, const char *value, unsigned flags) {
    Value v{}; v.string = value;
    return Register(name, 8, flags, v, Domain{});
}
}
void UI_RegisterDvarsWithBuildNumberDefault(bool drawBuildNumber) {
    ui_animSpeedScale = _Dvar_RegisterFloat("ui_animSpeedScale", 0x1p+0f, 0x1.0624dep-10f, 0x1.fffffep+127f, 0x80, "");
    Store_UI_RegisterString("ui_customModeName", "", 0x0);
    Store_UI_RegisterString("ui_customModeEditName", "", 0x0);
    Store_UI_RegisterString("ui_customModeDesc", "", 0x0);
    Store_UI_RegisterString("ui_customModeEditDesc", "", 0x0);
    ui_playlistPopulationRefreshTime = RegisterInt("ui_playlistPopulationRefreshTime", 0x000004b0u, 0x00000000u, 0x7fffffffu, 0x20000);
    _Dvar_RegisterFloat("tu_10_matchRatingSampleChance", 0x0p+0f, 0x0p+0f, 0x1p+0f, 0x0, "");
    ui_netSource = RegisterInt("ui_netSource", 0x00000001u, 0x00000000u, 0x00000003u, 0x0);
    ui_lastServerRefresh_0 = Store_UI_RegisterString("ui_lastServerRefresh_0", "", 0x0);
    ui_lastServerRefresh_1 = Store_UI_RegisterString("ui_lastServerRefresh_1", "", 0x0);
    ui_lastServerRefresh_2 = Store_UI_RegisterString("ui_lastServerRefresh_2", "", 0x0);
    ui_lastServerRefresh_3 = Store_UI_RegisterString("ui_lastServerRefresh_3", "", 0x0);
    ui_lastServerRefresh_4 = Store_UI_RegisterString("ui_lastServerRefresh_4", "", 0x0);
    ui_smallFont = _Dvar_RegisterFloat("ui_smallFont", 0x1.20418ap-1f, 0x0p+0f, 0x1p+0f, 0x1, "");
    ui_bigFont = _Dvar_RegisterFloat("ui_bigFont", 0x1.60418ap-1f, 0x0p+0f, 0x1p+0f, 0x1, "");
    ui_language = RegisterInt("ui_language", 0x00000000u, 0x00000000u, 0x00000010u, 0x0);
    ui_languagechanged = RegisterBool("ui_languagechanged", false, 0x0);
    ui_currentMap = RegisterInt("ui_currentMap", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    ui_preview = RegisterString("ui_preview", "", 0x0);
    RegisterString("ui_preview_map", "", 0x0);
    ui_mapPackChanged = RegisterBool("ui_mapPackChanged", false, 0x0);
    RegisterString("ui_load_index", "", 0x0);
    ui_prevTextEntryBox = RegisterInt("ui_prevTextEntryBox", 0xffffffffu, 0xffffffffu, 0x7fffffffu, 0x0);
    ui_ignoreMousePos = RegisterBool("ui_ignoreMousePos", false, 0x0);
    RegisterString("ui_favoriteAddress", "", 0x0);
    RegisterString("ui_favoriteName", "", 0x0);
    ui_drawBuildNumber = RegisterBool("ui_drawBuildNumber", drawBuildNumber, 0x0);
    RegisterString("ui_gametype_text", "", 0x0);
    ui_demoname = RegisterString("ui_demoname", "", 0x0);
    RegisterInt("ui_serverStatusTimeOut", 0x00001b58u, 0x00000000u, 0x7fffffffu, 0x0);
    ui_menuLvlNotify = RegisterBool("ui_menuLvlNotify", false, 0x80);
    selectedFriendName = RegisterString("selectedFriendName", "", 0x0);
    selectedFriendIndex = RegisterInt("selectedFriendIndex", 0x00000000u, 0x80000000u, 0x7fffffffu, 0x40);
    RegisterInt("selectedGroupIndex", 0x00000000u, 0x80000000u, 0x7fffffffu, 0x40);
    selectedPlayerXuid = RegisterString("selectedPlayerXuid", "", 0x0);
    selectedMenuItemIndex = RegisterInt("selectedMenuItemIndex", 0x00000000u, 0x80000000u, 0x7fffffffu, 0x0);
    RegisterString("highlightedPlayerName", "", 0x0);
    RegisterString("splitscreen_lobbyPlayerCount", "", 0x0);
    RegisterString("splitscreen_partyPlayerCount", "", 0x0);
    RegisterInt("tickerHeaderWidth", 0x00000000u, 0x80000000u, 0x7fffffffu, 0x0);
    ui_showDLCMaps = RegisterBool("ui_showDLCMaps", false, 0x0);
    RegisterBool("ui_showBryceMagic", false, 0x0);
    ui_scrollTextYOffset = _Dvar_RegisterFloat("ui_scrollTextYOffset", 0x0p+0f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollTextScrollRate = _Dvar_RegisterFloat("ui_scrollTextScrollRate", 0x1.99999ap-4f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollTextYOffsetResetDelay = _Dvar_RegisterFloat("ui_scrollTextYOffsetResetDelay", 0x1.4p+4f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollTextDelay = _Dvar_RegisterFloat("ui_scrollTextDelay", 0x1.4p+5f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollMOTDYOffset = _Dvar_RegisterFloat("ui_scrollMOTDYOffset", 0x0p+0f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_MOTDScrollRate = _Dvar_RegisterFloat("ui_MOTDScrollRate", 0x1.99999ap-4f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollMOTDDelay = _Dvar_RegisterFloat("ui_scrollMOTDDelay", 0x1.ep+4f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    ui_scrollMOTDYOffsetResetDelay = _Dvar_RegisterFloat("ui_scrollMOTDYOffsetResetDelay", 0x1.ep+3f, 0x0p+0f, 0x1.fffffep+127f, 0x0, "");
    RegisterBool("ui_partyFull", false, 0x0);
    ui_borderLowLightScale = _Dvar_RegisterFloat("ui_borderLowLightScale", 0x1.333334p-1f, 0x0p+0f, 0x1p+0f, 0x0, "");
    RegisterBool("ui_ethernetLinkActive", true, 0x0);
    ui_lobbypopup = RegisterString("ui_lobbypopup", "none", 0x0);
    ui_listboxIndex = RegisterInt("ui_listboxIndex", 0x00000000u, 0x80000000u, 0x7fffffffu, 0x0);
    ls_status = RegisterString("ls_status", "", 0x0);
    RegisterString("ui_favorite_message", "", 0x0);
    ui_keyboardtitle = RegisterString("ui_keyboardtitle", "", 0x0);
    ui_keyboard_dvar_edit = RegisterString("ui_keyboard_dvar_edit", "", 0x0);
    ls_demoauthor = RegisterString("ls_demoauthor", "", 0x0);
    scr_hardcore = RegisterInt("scr_hardcore", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    RegisterInt("ui_splitscreen", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    ui_choice_noaction = RegisterInt("ui_choice_noaction", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    ui_hud_showobjicons = RegisterInt("ui_hud_showobjicons", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    ui_hud_obituaries = RegisterInt("ui_hud_obituaries", 0x00000000u, 0x00000000u, 0x7fffffffu, 0x0);
    RegisterString("g_FactionName_Axis", "", 0x0);
    RegisterString("g_FactionName_Allies", "", 0x0);
    RegisterString("ui_keyboard_dvar_new", "", 0x0);
    tu7_ui_multiteam_compass_fix = RegisterBool("tu7_ui_multiteam_compass_fix", true, 0x20000);
    RegisterBool("ui_display_aar", false, 0x0);
    RegisterBool("ui_specops", false, 0x4000);
    RegisterBool("ui_hideminimap", false, 0x4000);
    RegisterBool("ui_readingSaveDevice", false, 0x40);
    ui_fxFontColor = Store_UI_RegisterColor("ui_fxFontColor", 0x1p+0f, 0x1p+0f, 0x1p+0f, 0x1p+0f, 0x0);
    ui_fxFontGlowColor = Store_UI_RegisterColor("ui_fxFontGlowColor", 0x1.19999ap-1f, 0x1.4cccccp-1f, 0x1.a8f5c2p-1f, 0x1p+0f, 0x0);
    ui_fxFontShadowColor = Store_UI_RegisterColor("ui_fxFontShadowColor", 0x0p+0f, 0x0p+0f, 0x0p+0f, 0x1p+0f, 0x0);
    ui_fxFontOutlineColor = Store_UI_RegisterColor("ui_fxFontOutlineColor", 0x1.51eb86p-1f, 0x1.8f5c28p-1f, 0x1p+0f, 0x1p+0f, 0x0);
    ui_drawOldUI = RegisterBool("ui_drawOldUI", true, 0x0);
    tu_allowDLCWeaponsByOwnership = RegisterBool("tu_allowDLCWeaponsByOwnership", true, 0x20000);
    RegisterInt("ui_loadscreenTimeout", 0x00001f40u, 0x00000000u, 0x7fffffffu, 0x0);
    Store_UI_RegisterRelatedDvars();
    ui_friendsListOpen = _Dvar_RegisterBool("ui_friendsListOpen", false, 0x0, "");
    ui_playerListOpen = _Dvar_RegisterBool("ui_playerListOpen", false, 0x0, "");
    _Dvar_RegisterBool("ui_playercardOpen", false, 0x0, "");
    Store_UI_RegisterTailDvars();
}
void UI_RegisterDvars() { UI_RegisterDvarsWithBuildNumberDefault(true); }
#endif
