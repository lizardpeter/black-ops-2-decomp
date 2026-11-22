// qcommon: common.cpp, cmd.cpp, and the types other files share from them.
#pragma once

enum errorParm_t
{
	ERR_FATAL = 0,
	ERR_DROP = 1,
	ERR_FROM_STARTUP = 2,
	ERR_SERVERDISCONNECT = 3,
	ERR_DISCONNECT = 4,
	ERR_SCRIPT = 5,
	ERR_SCRIPT_DROP = 6,
	ERR_LOCALIZATION = 7,
};

enum LocalClientNum_t
{
	INVALID_LOCAL_CLIENT = -1,
	LOCAL_CLIENT_0 = 0,
};

enum ControllerIndex_t
{
	INVALID_CONTROLLER_PORT = -1,
	CONTROLLER_INDEX_0 = 0,
};

enum ClientNum_t
{
	INVALID_CLIENT_INDEX = -1,
	CLIENT_INDEX_0 = 0,
	CLIENT_INDEX_COUNT = 18,
};

// com_clients.cpp
#define MAX_LOCAL_CLIENTS 1  // a macro: the bounds asserts' text shows it expanded

enum UIContextIndex_t
{
	INVALID_UI_CONTEXT = -1,
	UI_CONTEXT_0 = 0,
};

enum netsrc_t
{
	NS_NULL = -1,
	NS_CLIENT1 = 0,
};

enum LastInput_t
{
	LAST_INPUT_GAMEPAD = 0,
	LAST_INPUT_MOUSEMOVE = 1,
	LAST_INPUT_MOUSECLICK = 2,
	LAST_INPUT_KEYBOARD = 3,
};

struct ClientGameState
{
	int flags;
	LocalClientNum_t localClientNum;
	ControllerIndex_t controllerIndex;
	UIContextIndex_t uiContextIndex;
	netsrc_t networkID;
	LastInput_t lastInput;
};
static_assert(sizeof(ClientGameState) == 0x18, "ClientGameState");

extern ClientGameState clientGameStates[MAX_LOCAL_CLIENTS];
extern LocalClientNum_t primaryLocalClient;

LocalClientNum_t Com_LocalClients_GetPrimary();
LocalClientNum_t Com_ControllerIndex_GetLocalClientNum(ControllerIndex_t controllerIndex);
ControllerIndex_t Com_LocalClient_GetControllerIndex(LocalClientNum_t localClientNum);
UIContextIndex_t Com_LocalClient_GetUIContextIndex(LocalClientNum_t localClientNum);
void Com_LocalClient_LastInput_Set(LocalClientNum_t localClientNum, LastInput_t lastInput);
LastInput_t Com_LocalClient_LastInput_Get(LocalClientNum_t localClientNum);

// common.cpp
extern int com_frameTime;
void Com_Error(errorParm_t code, const char *fmt, ...);
void Com_Printf(int channel, const char *fmt, ...);
void Com_PrintWarning(int channel, const char *fmt, ...);
void Com_PrintError(int channel, const char *fmt, ...);

// sv_main_pc_mp.cpp (the dedicated server's answer)
bool Com_IsClientConsole();

// cmd.cpp
const char *Cmd_Argv(int argIndex);
struct itemDef_s;

struct CmdArgs
{
	int nesting;
	LocalClientNum_t localClientNum[8];
	ControllerIndex_t controllerIndex[8];
	itemDef_s *itemDef[8];
	int argshift[8];
	int argc[8];
	const char **argv[8];
	char textPool[8192];
	const char *argvPool[512];
	int usedTextPool[8];
	int totalUsedArgvPool;
	int totalUsedTextPool;
};

extern CmdArgs g_cmd_args[2];

// collision (cm_trace): per-thread trace state, defined in q_shared.cpp
struct cbrush_t;
struct cmodel_t;
struct PhysGeomList;

struct TraceCheckCount
{
	unsigned short global;
	unsigned short *partitions;
	unsigned short *brushes;
};

struct TraceThreadInfo
{
	TraceCheckCount checkcount;
	cbrush_t *box_brush;
	cmodel_t *box_model;
	PhysGeomList **geoms;
};
