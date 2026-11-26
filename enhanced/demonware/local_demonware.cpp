#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include <windows.h>
#include <string.h>

WINBASEAPI void WINAPI OutputDebugStringA(const char *);
WINBASEAPI DWORD WINAPI GetEnvironmentVariableW(const wchar_t *, wchar_t *, DWORD);

namespace
{
bool localDemonwareEnabled;
wchar_t identityAuditPath[260];
bool identityAuditEnabled;
}

void LocalDemonware_Enable()
{
	localDemonwareEnabled = true;
	LocalDemonware_Trace("T6 Demonware: loopback lobby transport enabled\n");
}

bool LocalDemonware_IsEnabled()
{
	return localDemonwareEnabled;
}

void LocalDemonware_Trace(const char *event)
{
	if (localDemonwareEnabled)
		OutputDebugStringA(event);
}

void IdentityAudit_Enable()
{
	constexpr DWORD capacity = sizeof(identityAuditPath) / sizeof(*identityAuditPath);
	const DWORD length = GetEnvironmentVariableW(L"T6_IDENTITY_AUDIT_FILE", identityAuditPath, capacity);
	identityAuditEnabled = length > 0 && length < capacity;
}

bool IdentityAudit_IsEnabled()
{
	return identityAuditEnabled;
}

void IdentityAudit_Trace(const char *event)
{
	if (!identityAuditEnabled)
		return;
	constexpr DWORD appendData = 4; // FILE_APPEND_DATA
	constexpr DWORD shareWrite = 2; // FILE_SHARE_WRITE
	constexpr DWORD openAlways = 4; // OPEN_ALWAYS
	const HANDLE file = CreateFileW(identityAuditPath, appendData, FILE_SHARE_READ | shareWrite,
	                                nullptr, openAlways, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return;
	DWORD written;
	WriteFile(file, event, static_cast<DWORD>(strlen(event)), &written, nullptr);
	CloseHandle(file);
}

#endif
