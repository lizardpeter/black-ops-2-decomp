// live/: the parts other modules use.
#pragma once

#include "qcommon/qcommon.h"

struct SessionData;

// live_dedicated.cpp
bool Live_Base_IsConnected(const ControllerIndex_t controllerIndex);

// live_win.cpp
bool Live_XUIDIsLocalPlayer(const unsigned __int64 xuid);

// live_sessions.cpp
ClientNum_t Session_FindRegisteredUser(const SessionData *session, unsigned __int64 xuid);
