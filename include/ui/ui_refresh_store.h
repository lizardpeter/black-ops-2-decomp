#pragma once
#ifdef T6_STORE
// Address adapter, not an original packed structure. Each pointer is bound to an
// independently observed global or table in the selected Store executable.
#define STORE_UI_REFRESH_GLOBALS(X) \
    X(clients) X(primaryClient) X(info) X(menuCount) X(focusCount) X(focusStack) \
    X(menu) X(menuType) X(transitionDuration) X(transitionStart) X(transitionActive) \
    X(refreshBlocked) X(refreshDeadline) X(refreshIndex) X(refreshLimit) \
    X(luiReady) X(luiState) X(luiDirty) X(luiEvent) X(luiLock) \
    X(buildDvar) X(friendsDvar) X(playerDvar) X(hudScale) X(placements) \
    X(hiresActive) X(hiresX) X(hiresY) X(versionMinor) X(versionPatch) X(white)
struct StoreUiRefreshGlobals {
#define FIELD(name) unsigned char *name;
    STORE_UI_REFRESH_GLOBALS(FIELD)
#undef FIELD
    unsigned char *controllerTable;
};
void Store_UI_RefreshBound(int localClient, const StoreUiRefreshGlobals &globals, bool zombies);
void UI_Refresh(int localClient);
#endif
