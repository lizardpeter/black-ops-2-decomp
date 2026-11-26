#ifdef T6_STORE
#include "enhanced/custom_games.h"
#include "enhanced/local_demonware.h"
#include "win32/win_gdk.h"
#include "qcommon/qcommon.h"
#ifdef T6_STORE_ZM
#include "universal/dvar.h"
#endif
#include <windows.h>

WINBASEAPI BOOL WINAPI GetModuleHandleExA(DWORD, const char *, HINSTANCE *);
WINBASEAPI DWORD WINAPI GetLastError();
WINBASEAPI DWORD WINAPI GetEnvironmentVariableW(const wchar_t *, wchar_t *, DWORD);
WINBASEAPI DWORD WINAPI GetFileAttributesW(const wchar_t *);
WINBASEAPI BOOL WINAPI CreateDirectoryW(const wchar_t *, void *);
WINBASEAPI int WINAPI WideCharToMultiByte(UINT, DWORD, const wchar_t *, int, char *, int, const char *, BOOL *);

extern "C" HINSTANCE t6_gdk_module;
extern "C" unsigned store_primaryDemonwareState;

long GdkUserCheckPrivilege(void *user, unsigned options, unsigned privilege, bool *hasPrivilege, unsigned *reason)
{
	constexpr unsigned multiplayer = 0xfe;
	constexpr unsigned lobbyConnected = 6;
	if (!LocalDemonware_IsEnabled() || user || options || privilege != multiplayer ||
	    store_primaryDemonwareState != lobbyConnected)
		return GdkUserCheckPrivilegeImpl(user, options, privilege, hasPrivilege, reason);

#ifdef T6_STORE_ZM
	// The Zombies menu reads this publisher setting when building its map selection.
	static const dvar_t *showNewestLeaderboards;
	if (!showNewestLeaderboards)
		showNewestLeaderboards = _Dvar_RegisterBool("ui_showNewestLeaderboards", false, 0,
		                                          "Show the newest Zombies leaderboards");
#endif
	if (hasPrivilege)
		*hasPrivilege = true;
	if (reason)
		*reason = 0;
	return 0;
}

extern "C" long GdkLoadRuntime()
{
	if (t6_gdk_module)
		return 0;
	constexpr DWORD fromAddress = 4; // GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
	if (!GetModuleHandleExA(fromAddress, reinterpret_cast<const char *>(&GdkLoadRuntime), &t6_gdk_module))
		return static_cast<long>(0x89240101u);
	CustomGames_ReadStartupOptions();
	return 0;
}

extern "C" char t6_store_save_path[260];
extern "C" unsigned char t6_store_save_ready;

extern "C" long InitializeApiImpl(unsigned long long, unsigned long long)
{
	if (t6_store_save_ready)
		return 0;

	// The original GDK startup normally schedules an asynchronous save-path callback.
	// Initialize its result directly for a local run, before the filesystem consumes it.
	const long result = GdkSavePath(nullptr, sizeof(t6_store_save_path), t6_store_save_path);
	if (result >= 0)
		t6_store_save_ready = 1;
	return result;
}

extern "C" long InitializeApiImplEx(unsigned long long version, unsigned long long sdk, unsigned long long)
{
	return InitializeApiImpl(version, sdk);
}

extern "C" void UninitializeApiImpl() {}
extern "C" void XErrorReport(unsigned int, const char *) {}
extern "C" long QueryApiImpl(const void *, const void *, void **result)
{
	if (!result)
		return static_cast<long>(0x80004003u); // E_POINTER
	*result = nullptr;
	return static_cast<long>(0x80004002u); // E_NOINTERFACE
}

// GDK save path wrapper used by the startup callback at 14034ECC0.
extern "C" long GdkSavePath(void *, __SIZE_TYPE__ capacity, char *path)
{
	constexpr long failure = static_cast<long>(0x80004005u); // E_FAIL
	if (!path || !capacity || capacity > 0x7FFFFFFFu)
		return static_cast<long>(0x80070057u); // E_INVALIDARG
	path[0] = '\0';
	wchar_t local[260];
	constexpr wchar_t suffix[] = L"\\T6Enhanced";
	DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", local, sizeof(local) / sizeof(*local));
	if (!length || length + sizeof(suffix) / sizeof(*suffix) > sizeof(local) / sizeof(*local))
		return failure;
	for (unsigned i = 0; i < sizeof(suffix) / sizeof(*suffix); ++i)
		local[length + i] = suffix[i];
	if (!CreateDirectoryW(local, nullptr))
	{
		if (GetLastError() != 183) // ERROR_ALREADY_EXISTS
			return failure;
		const DWORD attributes = GetFileAttributesW(local);
		if (attributes == 0xFFFFFFFFu || !(attributes & 0x10)) // FILE_ATTRIBUTE_DIRECTORY
			return failure;
	}
	if (!WideCharToMultiByte(65001, 0, local, -1, path, static_cast<int>(capacity), nullptr, nullptr))
	{
		path[0] = '\0';
		return failure;
	}
	return 0;
}

extern "C" int Sys_ReportUnhandledException(void *)
{
	Com_Error(ERR_FATAL, "Unhandled exception caught\n");
	return 1;
}
#endif
