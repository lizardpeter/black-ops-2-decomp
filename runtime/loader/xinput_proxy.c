// runtime/loader/xinput_proxy.c: XINPUT1_4.dll in the game folder, which loads the enhanced build.
//
// t6mp.exe and t6zm.exe import XInputGetState, XInputSetState and XInputGetCapabilities by ordinal (2, 3,
// 4), and Windows looks in the game's folder before the system folder for it. This DLL forwards those (and the rest
// of xinput1_4's exports) to the system's xinput1_4.dll, so controllers work as before. When the process is the
// supported Store executables (checked by PE timestamp and image size), it loads the matching enhanced DLL
// from the same folder and points each exe function the enhanced build replaces at its replacement
// (the DLL's t6_redirects table, tools/runtime_link.py): a 14-byte jump at the function's entry, written only if
// every replaced function still starts with the bytes the build saw. MP and ZM
// stop if their enhanced build cannot be installed. Other executables only get forwarding.
// Nothing on disk is changed; deleting the two DLLs restores the game.
//
// What happened is written to t6_enhanced.log next to the DLL.
#include "ui_build_default.h"

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int BOOL;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long SIZE_T;
typedef void *HANDLE;
typedef void *HMODULE;
typedef unsigned short WCHAR;
typedef void (*FARPROC)(void);

#define WINAPI __stdcall
#define DLL_PROCESS_ATTACH 1
#define PAGE_EXECUTE_READWRITE 0x40
#define GENERIC_WRITE 0x40000000
#define CREATE_ALWAYS 2
#define FILE_ATTRIBUTE_NORMAL 0x80
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define MAX_PATH 260

__declspec(dllimport) HMODULE WINAPI LoadLibraryW(const WCHAR *name);
__declspec(dllimport) FARPROC WINAPI GetProcAddress(HMODULE module, const char *name);
__declspec(dllimport) HMODULE WINAPI GetModuleHandleW(const WCHAR *name);
__declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HMODULE module, WCHAR *path, DWORD size);
__declspec(dllimport) DWORD WINAPI GetSystemDirectoryW(WCHAR *path, DWORD size);
__declspec(dllimport) BOOL WINAPI VirtualProtect(void *address, SIZE_T size, DWORD protect, DWORD *old);
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) BOOL WINAPI TerminateProcess(HANDLE process, DWORD exitCode);
__declspec(dllimport) void WINAPI OutputDebugStringA(const char *text);
__declspec(dllimport) BOOL WINAPI FlushInstructionCache(HANDLE process, const void *address, SIZE_T size);
__declspec(dllimport) BOOL WINAPI DisableThreadLibraryCalls(HMODULE module);
__declspec(dllimport) HANDLE WINAPI CreateFileW(const WCHAR *name, DWORD access, DWORD share, void *security,
                                               DWORD disposition, DWORD flags, HANDLE templ);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE file, const void *data, DWORD size, DWORD *written, void *overlapped);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE handle);

// -- the system xinput1_4.dll's exports, by ordinal; each export of this DLL jumps through its slot (xinput_stubs.s)
FARPROC xinput_slots[7];
static const WORD xinput_ordinals[7] = {2, 3, 4, 5, 7, 8, 10};

static DWORD WINAPI NotConnected(void) { return 1167; }  // ERROR_DEVICE_NOT_CONNECTED, if the system DLL is missing

static void LoadSystemXInput(void)
{
	WCHAR path[MAX_PATH];
	DWORD n = GetSystemDirectoryW(path, MAX_PATH - 20);
	const char *name = "\\xinput1_4.dll";
	for (DWORD i = 0; name[i]; ++i)
		path[n++] = (WCHAR)name[i];
	path[n] = 0;
	HMODULE real = n < MAX_PATH ? LoadLibraryW(path) : 0;
	for (int i = 0; i < 7; ++i)
	{
		FARPROC f = real ? GetProcAddress(real, (const char *)(ULONG_PTR)xinput_ordinals[i]) : 0;
		xinput_slots[i] = f ? f : (FARPROC)NotConnected;
	}
}

// -- the log
static HANDLE s_log = INVALID_HANDLE_VALUE;

static void Log(const char *text)
{
	DWORD n = 0, written;
	while (text[n])
		++n;
	if (s_log != INVALID_HANDLE_VALUE)
		WriteFile(s_log, text, n, &written, 0);
}

static void LogHex(unsigned long long v)
{
	char buf[19] = "0x";
	for (int i = 0; i < 16; ++i)
		buf[2 + i] = "0123456789ABCDEF"[(v >> (60 - 4 * i)) & 0xF];
	buf[18] = 0;
	Log(buf);
}

// -- the enhanced build
struct Redirect
{
	unsigned long long rva;     // the exe function
	unsigned long long target;  // its replacement in t6_enhanced.dll
	BYTE prologue[16];          // the exe function's first bytes when the build was made
};

struct Redirects
{
	unsigned long long count;
	struct Redirect entries[1];
};

struct RedirectIdentity
{
	DWORD timestamp;
	DWORD imageSize;
};

static BOOL Fail(const char *message)
{
	Log(message);
	OutputDebugStringA(message);
	if (s_log != INVALID_HANDLE_VALUE)
		CloseHandle(s_log);
	// Terminate even if the proxy was loaded dynamically or some jumps were already written.
	TerminateProcess(GetCurrentProcess(), 1114);  // ERROR_DLL_INIT_FAILED
	return 0;
}

static BOOL SameName(const WCHAR *path, const char *name)
{
	while (*name)
	{
		WCHAR ch = *path++;
		if (ch >= 'A' && ch <= 'Z')
			ch += 'a' - 'A';
		if (ch != (WCHAR)*name++)
			return 0;
	}
	return *path == 0;
}

static BOOL Install(HMODULE self)
{
	BYTE *exe = (BYTE *)GetModuleHandleW(0);
	const BYTE *nt = exe + *(const DWORD *)(exe + 0x3C);
	DWORD timestamp = *(const DWORD *)(nt + 8);
	DWORD imageSize = *(const DWORD *)(nt + 0x18 + 0x38);
	const char *dllName;
	const char *logName;
	const char *banner;
	if (timestamp == 0x6758BF4Fu && imageSize == 0x045AC000u)
	{
		dllName = "t6_enhanced.dll";
		logName = "t6_enhanced.log";
		banner = "t6_enhanced: t6mp.exe 1.1.8.0\r\n";
	}
	else if (timestamp == 0x6758BC50u && imageSize == 0x045AA000u)
	{
		dllName = "t6_enhanced_zm.dll";
		logName = "t6_enhanced_zm.log";
		banner = "t6_enhanced: t6zm.exe 1.1.8.0\r\n";
	}
	else
	{
		WCHAR exePath[MAX_PATH];
		DWORD end = GetModuleFileNameW(0, exePath, MAX_PATH);
		if (!end || end >= MAX_PATH)
			return Fail("T6 loader: could not identify the executable.\r\n");
		DWORD start = end;
		while (start && exePath[start - 1] != '\\')
			--start;
		if (!SameName(exePath + start, "t6mp.exe") && !SameName(exePath + start, "t6zm.exe"))
			return 1;
		return Fail("T6 loader: unsupported multiplayer or Zombies executable version.\r\n");
	}

	WCHAR path[MAX_PATH + 32];
	DWORD n = GetModuleFileNameW(self, path, MAX_PATH);
	if (!n || n >= MAX_PATH)
		return Fail("T6 loader: could not find the proxy DLL directory.\r\n");
	while (n && path[n - 1] != '\\')
		--n;
	for (DWORD i = 0; ; ++i)
	{
		path[n + i] = (WCHAR)logName[i];
		if (!logName[i])
			break;
	}
	s_log = CreateFileW(path, GENERIC_WRITE, 1, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	Log(banner);

	for (DWORD i = 0; ; ++i)
	{
		path[n + i] = (WCHAR)dllName[i];
		if (!dllName[i])
			break;
	}
	HMODULE enhanced = LoadLibraryW(path);
	if (!enhanced)
		return Fail("T6 loader: could not load the enhanced DLL.\r\n");
	const struct Redirects *table = (const struct Redirects *)GetProcAddress(enhanced, "t6_redirects");
	const struct RedirectIdentity *identity =
		(const struct RedirectIdentity *)GetProcAddress(enhanced, "t6_redirects_identity");
	if (!table || !identity)
		return Fail("T6 loader: the enhanced DLL has no compatible redirect table.\r\n");

	const BYTE *dll = (const BYTE *)enhanced;
	const BYTE *dllNt = dll + *(const DWORD *)(dll + 0x3C);
	DWORD dllSize = *(const DWORD *)(dllNt + 0x18 + 0x38);
	ULONG_PTR tableOffset = (ULONG_PTR)table - (ULONG_PTR)dll;
	ULONG_PTR identityOffset = (ULONG_PTR)identity - (ULONG_PTR)dll;
	if (dllSize < sizeof(*identity) || identityOffset > dllSize - sizeof(*identity) ||
	    tableOffset > identityOffset || identityOffset - tableOffset < sizeof(table->count))
		return Fail("T6 loader: invalid enhanced DLL redirect table.\r\n");
	ULONG_PTR entriesSize = identityOffset - tableOffset - sizeof(table->count);
	if (!table->count || entriesSize % sizeof(struct Redirect) ||
	    table->count != entriesSize / sizeof(struct Redirect))
		return Fail("T6 loader: invalid enhanced DLL redirect table.\r\n");
	if (identity->timestamp != timestamp || identity->imageSize != imageSize)
		return Fail("T6 loader: the enhanced DLL does not match this executable.\r\n");

	// P0017: the UI dvar registration was inlined, so there is no standalone function redirect.
	const struct BuildNumberDefaultSite *buildNumber = T6_BuildNumberDefaultSite(timestamp, imageSize);
	if (!buildNumber || buildNumber->rva > imageSize - sizeof(buildNumber->expected) ||
	    !T6_BuildNumberDefaultMatches(exe + buildNumber->rva, buildNumber))
		return Fail("T6 loader: unexpected UI build-number registration bytes.\r\n");

	// Check every entry before writing any jumps.
	for (unsigned long long i = 0; i < table->count; ++i)
	{
		const struct Redirect *r = &table->entries[i];
		if (r->rva > imageSize - sizeof(r->prologue) || r->target - (ULONG_PTR)dll >= dllSize)
			return Fail("T6 loader: invalid enhanced DLL redirect address.\r\n");
		if (r->rva < buildNumber->rva + sizeof(buildNumber->expected) &&
		    buildNumber->rva < r->rva + 14)
			return Fail("T6 loader: overlapping UI default and function redirect.\r\n");
		for (int b = 0; b < 16; ++b)
			if (exe[r->rva + b] != r->prologue[b])
				return Fail("T6 loader: the enhanced DLL does not match this executable.\r\n");
	}
	for (unsigned long long i = 0; i < table->count; ++i)
	{
		const struct Redirect *r = &table->entries[i];
		BYTE *code = exe + r->rva;
		DWORD old;
		if (!VirtualProtect(code, 14, PAGE_EXECUTE_READWRITE, &old))
			return Fail("T6 loader: could not make a redirect writable.\r\n");
		BYTE jump[14] = {0xFF, 0x25, 0, 0, 0, 0};  // jmp qword ptr [rip + 0]
		for (int b = 0; b < 8; ++b)
			jump[6 + b] = (BYTE)(r->target >> (8 * b));
		for (int b = 0; b < 14; ++b)
			code[b] = jump[b];
		if (!VirtualProtect(code, 14, old, &old))
			return Fail("T6 loader: could not restore executable page protection.\r\n");
		if (!FlushInstructionCache(GetCurrentProcess(), code, 14))
			return Fail("T6 loader: could not flush the instruction cache.\r\n");
		Log("replaced ");
		LogHex((unsigned long long)(ULONG_PTR)code);
		Log("\r\n");
	}
	// Change just the bool default from 1 to 0 before any game initialization.
	BYTE *defaultByte = exe + buildNumber->rva + T6_BUILD_NUMBER_DEFAULT_OFFSET;
	DWORD oldDefaultProtect;
	if (!VirtualProtect(defaultByte, 1, PAGE_EXECUTE_READWRITE, &oldDefaultProtect))
		return Fail("T6 loader: could not make UI default writable.\r\n");
	*defaultByte = 0;
	if (!VirtualProtect(defaultByte, 1, oldDefaultProtect, &oldDefaultProtect) ||
	    !FlushInstructionCache(GetCurrentProcess(), defaultByte, 1))
		return Fail("T6 loader: could not finalize UI default.\r\n");
	Log("ui_drawBuildNumber default: 0\r\n");
	Log("enhanced build active\r\n");
	if (s_log != INVALID_HANDLE_VALUE)
		CloseHandle(s_log);
	return 1;
}

BOOL WINAPI ProxyMain(HMODULE self, DWORD reason, void *reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(self);
		LoadSystemXInput();
		return Install(self);
	}
	return 1;
}
