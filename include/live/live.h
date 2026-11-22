// live/: the parts other modules use.
#pragma once

#include "qcommon/qcommon.h"

struct SessionData;

// live_dedicated.cpp
bool Live_Base_IsConnected(const ControllerIndex_t controllerIndex);

// live_win.cpp
#ifdef T6_STORE
// The Store revision keeps the system UI's state per controller (CL_CreateCmd 140194E20 reads the first byte of a
// 0x24-byte record per controller); the server's is one flag.
struct LiveSystemUi
{
	bool active;
	unsigned char unknown1[0x24 - 1];
};
bool Live_IsSystemUiActive(ControllerIndex_t controllerIndex);
#else
bool Live_IsSystemUiActive();
#endif
bool Live_XUIDIsLocalPlayer(const unsigned __int64 xuid);

// live_sessions.cpp
ClientNum_t Session_FindRegisteredUser(const SessionData *session, unsigned __int64 xuid);
