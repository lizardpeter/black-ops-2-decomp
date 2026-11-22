// runtime/loader/xinput_proxy.c: XINPUT1_4.dll in the game folder, which loads the enhanced build into t6mp.exe.
//
// t6mp.exe imports XInputGetState, XInputSetState and XInputGetCapabilities from XINPUT1_4.dll by ordinal (2, 3,
// 4), and Windows looks in the game's folder before the system folder for it. This DLL forwards those (and the rest
// of xinput1_4's exports) to the system's xinput1_4.dll, so controllers work as before. When the process is the
// Store t6mp.exe the enhanced build was made for (checked by its PE timestamp and image size), it also loads
// t6_enhanced.dll from the same folder and points each exe function the enhanced build replaces at its replacement
// (the DLL's t6_redirects table, tools/runtime_link.py): a 14-byte jump at the function's entry, written only if
// every replaced function still starts with the bytes the build saw. Any other exe (t6sp.exe, t6zm.exe, another
// version) only gets the forwarding. Nothing on disk is changed; deleting the two DLLs restores the game.
//
// What happened is written to t6_enhanced.log next to the DLL.
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
__declspec(dllimport) BOOL WINAPI FlushInstructionCache(HANDLE process, const void *address, SIZE_T size);
__declspec(dllimport) BOOL WINAPI DisableThreadLibraryCalls(HMODULE module);
__declspec(dllimport) HANDLE WINAPI CreateFileW(const WCHAR *name, DWORD access, DWORD share, void *security,
                                               DWORD disposition, DWORD flags, HANDLE templ);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE file, const void *data, DWORD size, DWORD *written, void *overlapped);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE handle);

// The build this loader patches: t6mp.exe 1.1.8.0 from the Store (docs/store-build.md)
#define T6MP_TIMESTAMP 0x6758BF4Fu
#define T6MP_IMAGE_SIZE 0x045AC000u

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

static void Install(HMODULE self)
{
	BYTE *exe = (BYTE *)GetModuleHandleW(0);
	const BYTE *nt = exe + *(const DWORD *)(exe + 0x3C);
	DWORD timestamp = *(const DWORD *)(nt + 8);
	DWORD imageSize = *(const DWORD *)(nt + 0x18 + 0x38);
	if (timestamp != T6MP_TIMESTAMP || imageSize != T6MP_IMAGE_SIZE)
		return;  // not the Store t6mp.exe 1.1.8.0: forwarding only, no log

	WCHAR path[MAX_PATH + 32];
	DWORD n = GetModuleFileNameW(self, path, MAX_PATH);
	while (n && path[n - 1] != '\\')
		--n;
	const char *logName = "t6_enhanced.log";
	for (DWORD i = 0; logName[i]; ++i)
		path[n + i] = (WCHAR)logName[i];
	path[n + 15] = 0;
	s_log = CreateFileW(path, GENERIC_WRITE, 1, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	Log("t6_enhanced: t6mp.exe 1.1.8.0\r\n");

	const char *dllName = "t6_enhanced.dll";
	for (DWORD i = 0; dllName[i]; ++i)
		path[n + i] = (WCHAR)dllName[i];
	path[n + 15] = 0;
	HMODULE enhanced = LoadLibraryW(path);
	const struct Redirects *table = enhanced ? (const struct Redirects *)GetProcAddress(enhanced, "t6_redirects") : 0;
	if (!table)
	{
		Log("t6_enhanced.dll is missing or has no redirect table: the game runs unchanged\r\n");
		goto done;
	}
	// all or nothing: every replaced function must still be what the build saw
	for (unsigned long long i = 0; i < table->count; ++i)
	{
		const struct Redirect *r = &table->entries[i];
		if (r->rva >= imageSize)
			goto mismatch;
		for (int b = 0; b < 16; ++b)
			if (exe[r->rva + b] != r->prologue[b])
				goto mismatch;
	}
	for (unsigned long long i = 0; i < table->count; ++i)
	{
		const struct Redirect *r = &table->entries[i];
		BYTE *code = exe + r->rva;
		DWORD old;
		if (!VirtualProtect(code, 14, PAGE_EXECUTE_READWRITE, &old))
		{
			Log("VirtualProtect failed: stopping\r\n");  // the ones before stay patched; each is complete
			goto done;
		}
		BYTE jump[14] = {0xFF, 0x25, 0, 0, 0, 0};  // jmp qword ptr [rip + 0]
		for (int b = 0; b < 8; ++b)
			jump[6 + b] = (BYTE)(r->target >> (8 * b));
		for (int b = 0; b < 14; ++b)
			code[b] = jump[b];
		VirtualProtect(code, 14, old, &old);
		FlushInstructionCache(GetCurrentProcess(), code, 14);
		Log("replaced ");
		LogHex((unsigned long long)(ULONG_PTR)code);
		Log("\r\n");
	}
	Log("enhanced build active\r\n");
	goto done;
mismatch:
	Log("t6_enhanced.dll doesn't match this t6mp.exe: nothing replaced, the game runs unchanged\r\n");
done:
	if (s_log != INVALID_HANDLE_VALUE)
		CloseHandle(s_log);
}

BOOL WINAPI ProxyMain(HMODULE self, DWORD reason, void *reserved)
{
	(void)reserved;
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(self);
		LoadSystemXInput();
		Install(self);
	}
	return 1;
}
