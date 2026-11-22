// client/cl_keys.cpp (CoDMPServer_PC: cl_keys.obj)
//
// Key bindings and catchers. So far the catcher test.
#define T6_SOURCE "client/cl_keys.cpp"

#include "client/client.h"

// 004FEB60  client/cl_keys.cpp:3788 (inlined in the Store build)
// Whether any of mask's catchers (console, UI, chat, ...) takes the client's keys. No client: none.
bool Key_IsCatcherActive(LocalClientNum_t localClientNum, int mask)
{
	if (localClientNum == INVALID_LOCAL_CLIENT)
		return false;
	return (CL_GetLocalClientUIGlobals(localClientNum)->keyCatchers & mask) != 0;
}
