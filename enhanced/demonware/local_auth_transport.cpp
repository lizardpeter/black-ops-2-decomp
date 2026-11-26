#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include <windows.h>

extern "C" __declspec(dllimport) void *WINAPI WinHttpOpen(const wchar_t *, DWORD, const wchar_t *, const wchar_t *, DWORD);
extern "C" __declspec(dllimport) void *WINAPI WinHttpConnect(void *, const wchar_t *, unsigned short, DWORD);
extern "C" __declspec(dllimport) void *WINAPI WinHttpOpenRequest(void *, const wchar_t *, const wchar_t *,
                                                               const wchar_t *, const wchar_t *, const wchar_t **, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSetOption(void *, DWORD, void *, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSetTimeouts(void *, int, int, int, int);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSendRequest(void *, const wchar_t *, DWORD, void *, DWORD, DWORD,
                                                               __UINTPTR_TYPE__);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpReceiveResponse(void *, void *);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpQueryHeaders(void *, DWORD, const wchar_t *, void *, DWORD *, DWORD *);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpReadData(void *, void *, DWORD, DWORD *);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpCloseHandle(void *);

namespace
{
struct HttpHandle
{
	void *value;

	~HttpHandle()
	{
		if (value)
			WinHttpCloseHandle(value);
	}
};

struct AuthRequest
{
	unsigned char prefix[0x14];
	DWORD status;
	unsigned char beforeDownload[8];
	char *download;
	DWORD downloadCapacity;
	unsigned char padding[4];
	void *upload;
	DWORD uploadLength;
	unsigned char beforeDownloadLength[0x60 - 0x3c];
	DWORD downloadLength;
	DWORD sentLength;
	unsigned char beforeUrl[0x88 - 0x68];
	wchar_t url[0x800];
};

static_assert(__builtin_offsetof(AuthRequest, download) == 0x20);
static_assert(__builtin_offsetof(AuthRequest, upload) == 0x30);
static_assert(__builtin_offsetof(AuthRequest, downloadLength) == 0x60);
static_assert(__builtin_offsetof(AuthRequest, url) == 0x88);
}

bool LocalDemonware_SendAuthRequest(void *data)
{
	auto &auth = *static_cast<AuthRequest *>(data);
	for (unsigned i = 0; i < sizeof(demonwareAuthUrl); ++i)
	{
		if (auth.url[i] != demonwareAuthUrl[i])
			return false;
	}
	if (!auth.upload || !auth.uploadLength || !auth.download || auth.downloadCapacity < 2)
		return false;

	constexpr DWORD noProxy = 1;
	const HttpHandle session{WinHttpOpen(L"T6 local Demonware", noProxy, nullptr, nullptr, 0)};
	if (!session.value || !WinHttpSetTimeouts(session.value, 2000, 2000, 2000, 2000))
		return false;
	const HttpHandle connection{WinHttpConnect(session.value, L"127.0.0.1", demonwareAuthPort, 0)};
	if (!connection.value)
		return false;
	const HttpHandle request{WinHttpOpenRequest(connection.value, L"POST", L"/auth/", nullptr, nullptr, nullptr, 0)};
	if (!request.value)
		return false;

	constexpr DWORD disableFeature = 63;
	DWORD disabled = 0x1 | 0x2 | 0x4; // cookies, redirects and automatic authentication
	constexpr wchar_t headers[] = L"Content-Type: application/json\r\nAuthorization: T6-Local\r\n";
	if (!WinHttpSetOption(request.value, disableFeature, &disabled, sizeof(disabled)) ||
	    !WinHttpSendRequest(request.value, headers, ~0u, auth.upload, auth.uploadLength, auth.uploadLength, 0) ||
	    !WinHttpReceiveResponse(request.value, nullptr))
		return false;

	constexpr DWORD numericStatusCode = 0x20000013;
	DWORD statusBytes = sizeof(auth.status);
	if (!WinHttpQueryHeaders(request.value, numericStatusCode, nullptr, &auth.status, &statusBytes, nullptr))
		return false;
	auth.sentLength = auth.uploadLength;
	for (DWORD total = 0; total < auth.downloadCapacity - 1;)
	{
		DWORD read;
		if (!WinHttpReadData(request.value, auth.download + total, auth.downloadCapacity - 1 - total, &read))
			return false;
		if (!read)
		{
			auth.download[total] = '\0';
			auth.downloadLength = total;
			return true;
		}
		total += read;
	}
	return false;
}
#endif
