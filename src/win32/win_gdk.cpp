#ifdef T6_STORE
#include "win32/win_gdk.h"
#include "qcommon/qcommon.h"
#include <windows.h>

WINBASEAPI HINSTANCE WINAPI LoadLibraryExA(const char *, HANDLE, DWORD);
extern "C" __declspec(dllimport) int WINAPI ReportFault(void *, DWORD);
extern "C" HINSTANCE t6_gdk_module;
WINBASEAPI void WINAPI AcquireSRWLockExclusive(void *);
WINBASEAPI void WINAPI ReleaseSRWLockExclusive(void *);
WINBASEAPI void WINAPI DebugBreak();
WINBASEAPI BOOL WINAPI FreeLibrary(HINSTANCE);

namespace
{
using Query = long (WINAPI *)(const void *, const void *, void **);
using Initialize = long (WINAPI *)(unsigned, unsigned);
using InitializeEx = long (WINAPI *)(unsigned long long, unsigned long long, unsigned long long);
using ErrorReport = void (WINAPI *)(unsigned, const char *);
using Uninitialize = void (WINAPI *)();

struct RuntimeDispatch
{
	void *lock;
	unsigned char initialized;
	unsigned char implicit;
	HINSTANCE module;
	Query query;
	Initialize initialize;
	InitializeEx initializeEx;
	ErrorReport report;
	Uninitialize uninitialize;
};

static_assert(__builtin_offsetof(RuntimeDispatch, module) == 0x10);
static_assert(__builtin_offsetof(RuntimeDispatch, uninitialize) == 0x38);
}

using Proc = void (WINAPI *)();
WINBASEAPI Proc WINAPI GetProcAddress(HINSTANCE, const char *);
extern "C" RuntimeDispatch t6_gdk_dispatch;

extern "C" long GdkLoadRuntime()
{
	if (t6_gdk_module)
		return 0;
	constexpr DWORD systemDirectory = 0x800; // LOAD_LIBRARY_SEARCH_SYSTEM32
	t6_gdk_module = LoadLibraryExA("xgameruntime.dll", nullptr, systemDirectory);
	return t6_gdk_module ? 0 : static_cast<long>(0x89240101u);
}

extern "C" long GdkInitializeRuntime(unsigned long long flags)
{
	auto &runtime = t6_gdk_dispatch;
	AcquireSRWLockExclusive(&runtime.lock);
	long status = runtime.module ? 0 : GdkLoadRuntime();
	if (status >= 0)
	{
		if (!runtime.query)
			runtime.query = reinterpret_cast<Query>(GetProcAddress(runtime.module, "QueryApiImpl"));
		if (!runtime.initialize)
			runtime.initialize = reinterpret_cast<Initialize>(GetProcAddress(runtime.module, "InitializeApiImpl"));
		if (!runtime.initializeEx)
			runtime.initializeEx = reinterpret_cast<InitializeEx>(GetProcAddress(runtime.module, "InitializeApiImplEx"));
		if (!runtime.report)
			runtime.report = reinterpret_cast<ErrorReport>(GetProcAddress(runtime.module, "XErrorReport"));
		if (!runtime.uninitialize)
			runtime.uninitialize = reinterpret_cast<Uninitialize>(GetProcAddress(runtime.module, "UninitializeApiImpl"));

		const bool implicit = (flags & 1) != 0;
		if (!runtime.query || (!runtime.initialize && !runtime.initializeEx) || !runtime.report || !runtime.uninitialize)
			status = static_cast<long>(0x89240101u);
		else if (!runtime.initializeEx && implicit)
		{
			status = static_cast<long>(0x89240100u);
			runtime.report(0x89240100u, "XGameRuntimeInitialize must be called before making this call.");
		}
		else if (!runtime.initialized || (runtime.implicit && !implicit))
		{
			status = runtime.initializeEx ? runtime.initializeEx(0x633600002711ull, 0x633600000c6dull, flags | 2)
			                              : runtime.initialize(0x6336, 0x2711);
			if (status >= 0)
			{
				runtime.initialized = 1;
				runtime.implicit = implicit;
			}
		}
	}
	// Shared Windows KUSER_SHARED_DATA.KdDebuggerEnabled byte.
	if (status < 0 && (*reinterpret_cast<const volatile unsigned char *>(0x7ffe02d4) & 3) == 3)
		DebugBreak();
	ReleaseSRWLockExclusive(&runtime.lock);
	return status;
}

extern "C" long GdkQueryApi(const void *classId, const void *interfaceId, void **result)
{
	auto &runtime = t6_gdk_dispatch;
	if (!runtime.initialized)
	{
		const long status = GdkInitializeRuntime(1);
		if (status < 0)
			return status;
	}
	const long status = runtime.query(classId, interfaceId, result);
	if (status != static_cast<long>(0x80040154u))
		return status;
	if (runtime.report)
		runtime.report(0x89240107u, "XGameRuntime is missing required dependencies, run GamingRepair.exe to resolve.");
	return static_cast<long>(0x89240107u);
}

extern "C" void GdkUninitializeRuntime()
{
	auto &runtime = t6_gdk_dispatch;
	AcquireSRWLockExclusive(&runtime.lock);
	if (runtime.initialized && !runtime.implicit && runtime.uninitialize)
		runtime.uninitialize();
	runtime.query = nullptr;
	runtime.initialize = nullptr;
	runtime.initializeEx = nullptr;
	runtime.report = nullptr;
	runtime.uninitialize = nullptr;
	runtime.initialized = 0;
	runtime.implicit = 0;
	if (runtime.module)
	{
		FreeLibrary(runtime.module);
		runtime.module = nullptr;
	}
	ReleaseSRWLockExclusive(&runtime.lock);
}

namespace
{
alignas(4) constexpr unsigned char userClass[] =
	{0x77, 0xd1, 0xac, 0x01, 0xf9, 0x91, 0x63, 0x47, 0xa3, 0x8e, 0xcc, 0xbb, 0x55, 0xce, 0x32, 0xe0};
alignas(4) constexpr unsigned char userInterface[] =
	{0xe3, 0x15, 0x94, 0x07, 0x27, 0x67, 0x7f, 0x43, 0x8e, 0x9d, 0x8f, 0x8f, 0x9b, 0x24, 0x39, 0xf7};

void ReleaseInterface(void *object, void **vtable)
{
	using Release = unsigned long (WINAPI *)(void *);
	reinterpret_cast<Release>(vtable[2])(object);
}

template<unsigned Slot, class First, class Second>
long CallUserMethod(First first, Second second)
{
	void *object;
	const long queryStatus = GdkQueryApi(userClass, userInterface, &object);
	if (queryStatus < 0)
		return queryStatus;
	auto *vtable = *static_cast<void ***>(object);
	using Method = long (WINAPI *)(void *, First, Second);
	const long status = reinterpret_cast<Method>(vtable[Slot])(object, first, second);
	ReleaseInterface(object, vtable);
	return status;
}
}

extern "C" long GdkUserAddAsync(unsigned options, void *asyncBlock)
{
	return CallUserMethod<7>(options, asyncBlock);
}

extern "C" long GdkUserAddResult(void *asyncBlock, void **user)
{
	return CallUserMethod<8>(asyncBlock, user);
}

extern "C" long GdkUserDuplicateHandle(void *user, void **duplicate)
{
	return CallUserMethod<3>(user, duplicate);
}

extern "C" long GdkUserGetId(void *user, unsigned long long *id)
{
	return CallUserMethod<11>(user, id);
}

long GdkUserCheckPrivilege(void *user, unsigned options, unsigned privilege, bool *hasPrivilege, unsigned *reason)
{
	return GdkUserCheckPrivilegeImpl(user, options, privilege, hasPrivilege, reason);
}

long GdkUserCheckPrivilegeImpl(void *user, unsigned options, unsigned privilege, bool *hasPrivilege, unsigned *reason)
{
	void *object;
	const long queryStatus = GdkQueryApi(userClass, userInterface, &object);
	if (queryStatus < 0)
		return queryStatus;
	auto *vtable = *static_cast<void ***>(object);
	using Method = long (WINAPI *)(void *, void *, unsigned, unsigned, bool *, unsigned *);
	const long result = reinterpret_cast<Method>(vtable[20])(object, user, options, privilege, hasPrivilege, reason);
	ReleaseInterface(object, vtable);
	return result;
}

extern "C" void GdkUserCloseHandle(void *user)
{
	void *object;
	if (GdkQueryApi(userClass, userInterface, &object) >= 0)
	{
		auto *vtable = *static_cast<void ***>(object);
		using Close = void (WINAPI *)(void *, void *);
		reinterpret_cast<Close>(vtable[4])(object, user);
		ReleaseInterface(object, vtable);
	}
}

namespace
{
alignas(4) constexpr unsigned char threadingId[] =
	{0xcb, 0x7d, 0x3b, 0x07, 0xcf, 0x1f, 0x30, 0x40, 0x94, 0xbe, 0xe3, 0xc9, 0xeb, 0x62, 0x34, 0x28};

long QueryThreading(void **object)
{
	return GdkQueryApi(threadingId, threadingId, object);
}
}

extern "C" void GdkPrepareThreading()
{
	void *object;
	if (QueryThreading(&object) >= 0)
	{
		auto *vtable = *static_cast<void ***>(object);
		reinterpret_cast<void (WINAPI *)(void *)>(vtable[29])(object);
		ReleaseInterface(object, vtable);
	}
}

extern "C" long GdkTaskQueueCreate(unsigned workMode, unsigned completionMode, void **queue)
{
	GdkPrepareThreading();
	void *object;
	long status = QueryThreading(&object);
	if (status >= 0)
	{
		auto *vtable = *static_cast<void ***>(object);
		using Create = long (WINAPI *)(void *, unsigned, unsigned, void **);
		status = reinterpret_cast<Create>(vtable[12])(object, workMode, completionMode, queue);
		ReleaseInterface(object, vtable);
	}
	return status;
}

extern "C" void GdkTaskQueueClose(void *queue)
{
	void *object;
	if (QueryThreading(&object) >= 0)
	{
		auto *vtable = *static_cast<void ***>(object);
		using Close = void (WINAPI *)(void *, void *);
		reinterpret_cast<Close>(vtable[17])(object, queue);
		ReleaseInterface(object, vtable);
	}
}

bool sub_1407893BC(void *queue, int port, int timeout)
{
	void *object;
	if (QueryThreading(&object) < 0)
		return false;
	auto *vtable = *static_cast<void ***>(object);
	using Dispatch = bool (WINAPI *)(void *, void *, int, int);
	const bool handled = reinterpret_cast<Dispatch>(vtable[16])(object, queue, port, timeout);
	ReleaseInterface(object, vtable);
	return handled;
}

extern "C" int HttpQueueCallback(void *queue, int port, void *context, GdkTaskQueueCallback callback)
{
	void *object;
	const long status = QueryThreading(&object);
	if (status < 0)
		return status;
	auto *vtable = *static_cast<void ***>(object);
	using Submit = int (WINAPI *)(void *, void *, int, void *, GdkTaskQueueCallback);
	const int result = reinterpret_cast<Submit>(vtable[18])(object, queue, port, context, callback);
	ReleaseInterface(object, vtable);
	return result;
}

extern "C" long GdkSavePath(void *context, __SIZE_TYPE__ capacity, char *path)
{
	alignas(4) constexpr unsigned char classId[] =
		{0x58, 0x3f, 0x4c, 0x70, 0x29, 0xe6, 0xc2, 0x4c, 0xb1, 0x97, 0x30, 0x51, 0x1b, 0x99, 0x6f, 0xe2};
	alignas(4) constexpr unsigned char interfaceId[] =
		{0x58, 0x3f, 0x4c, 0x70, 0x29, 0xe6, 0xc2, 0x4c, 0xb1, 0x97, 0x30, 0x51, 0x1b, 0x99, 0x6e, 0xe2};
	void *object;
	long result = GdkQueryApi(classId, interfaceId, &object);
	if (result >= 0)
	{
		auto *vtable = *static_cast<void ***>(object);
		using GetPath = long (WINAPI *)(void *, void *, __SIZE_TYPE__, char *);
		result = reinterpret_cast<GetPath>(vtable[31])(object, context, capacity, path);
		ReleaseInterface(object, vtable);
	}
	return result;
}

extern "C" int Sys_ReportUnhandledException(void *exception)
{
	ReportFault(exception, 0);
	Com_Error(ERR_FATAL, "Unhandled exception caught\n");
	return 1;
}
#endif
