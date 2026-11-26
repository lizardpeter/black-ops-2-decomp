#ifdef T6_STORE
#include "enhanced/custom_games.h"
#include "enhanced/local_demonware.h"
#include "qcommon/qcommon.h"
#include <windows.h>

WINBASEAPI const wchar_t *WINAPI GetCommandLineW();
WINBASEAPI void *WINAPI LocalFree(void *);
extern "C" __declspec(dllimport) wchar_t **WINAPI CommandLineToArgvW(const wchar_t *, int *);

namespace
{
enum class LuaType : int
{
	Boolean = 1,
	Number = 3,
	Table = 5,
	InterpretedFunction = 9,
	CFunction = 10,
};

struct LuaValue
{
	LuaType type;
	int reserved;
	union
	{
		float number;
		int boolean;
	};
	int padding;
};

struct LuaState
{
	unsigned char header[0x48];
	LuaValue *top;
	LuaValue *base;
};

static_assert(sizeof(LuaValue) == 16);

enum class Startup
{
	Disabled,
	WaitingForLui,
	ControllerQueued,
};

Startup startup = Startup::Disabled;
bool localHostEnabled;
}

extern "C" unsigned int store_sessionModeFlags;
extern "C" const void *store_playerStatsSchema;
extern "C" bool StoreLiveStatsAreFetched(ControllerIndex_t controller, int source);
extern "C" void StoreLiveStatsReset(ControllerIndex_t controller, bool versionChanged, int reason);
extern "C" LuaState *store_luiLuaState;
extern "C" CRITICAL_SECTION store_luiLock;
extern "C" void StoreHksGetField(LuaState *state, int index, const char *name);
extern "C" int StoreHksProtectedCall(LuaState *state, int args, int results, int errorHandler);
extern "C" void StoreLuiTransitionToMenu(int controller, bool leaveGameLobby, const char *menu,
                                        int frontendState, const void *maxPlayers,
                                        const void *maxLocalPlayers, bool resetUi);
void Cbuf_InsertText(LocalClientNum_t, const char *);

namespace
{
LuaType ValueType(const LuaValue &value)
{
	return static_cast<LuaType>(static_cast<int>(value.type) & 0xf);
}

bool FindLuaFunction(LuaState *state, const char *table, const char *field)
{
	constexpr int globals = -10002;
	StoreHksGetField(state, globals, table);
	if (ValueType(state->top[-1]) != LuaType::Table)
		return false;
	StoreHksGetField(state, -1, field);
	const LuaType type = ValueType(state->top[-1]);
	return type == LuaType::InterpretedFunction || type == LuaType::CFunction;
}

bool CallLuaFunction(LuaState *state, const char *table, const char *field, LuaValue argument)
{
	const auto depth = state->top - state->base;
	bool success = FindLuaFunction(state, table, field);
	if (success)
	{
		*state->top++ = argument;
		success = StoreHksProtectedCall(state, 1, 0, 0) == 0;
	}
	state->top = state->base + depth;
	return success;
}
}

bool CustomGames_UsesLocalHost()
{
	constexpr unsigned int offline = 1;
	constexpr unsigned int systemLink = 2;
	constexpr unsigned int onlineGame = 4;
	constexpr unsigned int privateMatch = 8;
	constexpr unsigned int sessionModes = offline | systemLink | onlineGame | privateMatch;
	return localHostEnabled && (store_sessionModeFlags & sessionModes) == (offline | privateMatch);
}

void CustomGames_ReadStartupOptions()
{
	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv)
		return;
	for (int i = 1; i < argc; ++i)
	{
		const wchar_t *arg = argv[i];
		const auto matches = [arg](const wchar_t *option)
		{
			unsigned j = 0;
			while (arg[j] && arg[j] == option[j])
				++j;
			return arg[j] == option[j];
		};
		if (matches(L"--custom-games"))
			startup = Startup::WaitingForLui;
		else if (matches(L"--local-demonware"))
			LocalDemonware_Enable();
		else if (matches(L"--audit-identity"))
			IdentityAudit_Enable();
	}
	LocalFree(argv);
}

void CustomGames_UpdateStartup()
{
	if (startup == Startup::Disabled || !store_luiLuaState)
		return;

	EnterCriticalSection(&store_luiLock);
	LuaState *state = store_luiLuaState;
	if (startup == Startup::WaitingForLui)
	{
		const auto depth = state->top - state->base;
		const bool ready = FindLuaFunction(state, "CoD", "SwitchToPrivateLobby");
		state->top = state->base + depth;
		LeaveCriticalSection(&store_luiLock);
		if (ready)
		{
			Cbuf_InsertText(LOCAL_CLIENT_0, "setclientbeingusedandprimaryandactive");
			startup = Startup::ControllerQueued;
		}
		return;
	}

	const LuaValue controller{LuaType::Number, 0, {.number = 0.0f}, 0};
	const LuaValue enabled{LuaType::Boolean, 0, {.boolean = 1}, 0};
	const LuaValue disabled{LuaType::Boolean, 0, {.boolean = 0}, 0};
	const bool ready = CallLuaFunction(state, "CoD", "SwitchToPrivateLobby", controller) &&
	                   CallLuaFunction(state, "Engine", "SessionModeSetOnlineGame", disabled) &&
	                   CallLuaFunction(state, "Engine", "SessionModeSetOffline", enabled);
	LeaveCriticalSection(&store_luiLock);

	startup = Startup::Disabled;
	if (!ready)
		return;

	// Weapon and class previews use the native stats schema even in a private match.
	// Initialize a new offline profile only when controller zero has no stats.
	constexpr int primaryStatsSource = 0;
	constexpr int newOfflineUser = 5; // STATSRESET_NEW_OFFLINE_USER
	if (store_playerStatsSchema && !StoreLiveStatsAreFetched(CONTROLLER_INDEX_0, primaryStatsSource))
		StoreLiveStatsReset(CONTROLLER_INDEX_0, false, newOfflineUser);

	localHostEnabled = true;
	constexpr int mainLobbyFrontendState = 9;
	StoreLuiTransitionToMenu(0, false, "PrivateOnlineGameLobby", mainLobbyFrontendState, nullptr, nullptr, false);
	// Session-mode changes can release the local client established while the frontend was loading.
	Cbuf_InsertText(LOCAL_CLIENT_0, "setclientbeingusedandprimaryandactive; xstartpartyhost");
}
#endif
