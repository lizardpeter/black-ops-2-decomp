// qcommon/com_clients.cpp (CoDMPServer_PC: com_clients.obj)
//
// The local clients: which controller each one uses and what it last gave input with. The Store build inlines all of
// these into their callers.
#define T6_ASSERT_DIR "c:\\t6\\code\\src\\qcommon\\"
#define T6_ASSERT_FILE T6_ASSERT_DIR "com_clients.cpp"
#define T6_SOURCE "qcommon/com_clients.cpp"

#include "t6_assert.h"
#include "t6_static.h"
#include "qcommon/qcommon.h"
#include "win32/win_shared.h"

void CG_UpdateVehicleBindings(LocalClientNum_t localClientNum);  // cgame/cg_camera.cpp

T6_GLOBAL ClientGameState clientGameStates[MAX_LOCAL_CLIENTS];
T6_GLOBAL LocalClientNum_t primaryLocalClient;

// 006B5C90  qcommon/com_clients.cpp:190
UIContextIndex_t Com_LocalClient_GetUIContextIndex(LocalClientNum_t localClientNum)
{
	bcassert_at(191, localClientNum, MAX_LOCAL_CLIENTS);
	assert_at(192, clientGameStates[localClientNum].localClientNum == localClientNum);
	if (clientGameStates[localClientNum].localClientNum != localClientNum)
		return INVALID_UI_CONTEXT;
	return clientGameStates[localClientNum].uiContextIndex;
}

// 006B5D10  qcommon/com_clients.cpp:201
ControllerIndex_t Com_LocalClient_GetControllerIndex(LocalClientNum_t localClientNum)
{
	bcassert_at(202, localClientNum, MAX_LOCAL_CLIENTS);
	assert_at(203, clientGameStates[localClientNum].localClientNum == localClientNum);
	if (clientGameStates[localClientNum].localClientNum != localClientNum)
		return INVALID_CONTROLLER_PORT;
	return clientGameStates[localClientNum].controllerIndex;
}

// 006B5EC0  qcommon/com_clients.cpp:265
LocalClientNum_t Com_LocalClients_GetPrimary()
{
	assertf_at(267, primaryLocalClient != INVALID_LOCAL_CLIENT, "%s",
	           "Someone asked for the primary client and it was not set\n");
	return primaryLocalClient;
}

// 006B6300  qcommon/com_clients.cpp:615
// A change of input device only takes after it has persisted for 200 ms. Switching to or from the gamepad rebinds the
// vehicle controls.
void Com_LocalClient_LastInput_Set(LocalClientNum_t localClientNum, LastInput_t lastInput)
{
	T6_STATIC int resetTime T6_AT("Com_LocalClient_LastInput_Set", "resetTime");

	if (clientGameStates[localClientNum].lastInput == lastInput)
	{
		resetTime = 0;
		return;
	}
	int now = Sys_Milliseconds();
	if (!resetTime)
	{
		resetTime = now + 200;
		return;
	}
	if (resetTime > now)
		return;
	LastInput_t previous = clientGameStates[localClientNum].lastInput;
	resetTime = 0;
	clientGameStates[localClientNum].lastInput = lastInput;
	if (previous == LAST_INPUT_GAMEPAD || lastInput == LAST_INPUT_GAMEPAD)
		CG_UpdateVehicleBindings(localClientNum);
}

// 006B6380  qcommon/com_clients.cpp:671
LastInput_t Com_LocalClient_LastInput_Get(LocalClientNum_t localClientNum)
{
	return clientGameStates[localClientNum].lastInput;
}
