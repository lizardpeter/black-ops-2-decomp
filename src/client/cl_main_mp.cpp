#ifdef T6_STORE
#include "qcommon/qcommon.h"

extern "C" unsigned int store_sessionModeFlags;
extern "C" unsigned int store_143ED8224;
extern "C" int store_141EF91E8;
extern "C" bool store_141EF8675;
extern "C" void StoreSetControllerInUse(ControllerIndex_t controller, bool inUse);
extern "C" void sub_1406F52E0(ControllerIndex_t controller, bool enabled);
extern "C" void sub_1405A68D0(ControllerIndex_t controller);
extern "C" void sub_1401B3900(const char *map);

void CL_MapLoading_ConfigureDevmapParty(const char *map)
{
	constexpr unsigned int systemLink = 2;
	constexpr unsigned int onlineGame = 4;
	if ((store_sessionModeFlags & (systemLink | onlineGame)) || (store_143ED8224 & 4))
		return;

	StoreSetControllerInUse(CONTROLLER_INDEX_0, true);
	sub_1406F52E0(CONTROLLER_INDEX_0, true);
	store_141EF91E8 = 0;
	sub_1405A68D0(CONTROLLER_INDEX_0);
	if (store_141EF8675)
		Com_Error(ERR_FATAL, "\x15" "Com_GameMode_SetMode: Can not set the mode when in backup block!");
	store_sessionModeFlags |= systemLink;
	sub_1401B3900(map);
}

void CL_MapLoading_SetupDevmapParty(const char *map)
{
	CL_MapLoading_ConfigureDevmapParty(map);
}
#endif
