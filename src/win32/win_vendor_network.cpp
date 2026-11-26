#ifdef T6_STORE
#include "win32/win_vendor_network.h"
#include "win32/win_shared.h"

#include <string.h>
#include <windows.h>

namespace
{
using Socket = __UINTPTR_TYPE__;

struct Ipv4Endpoint
{
	unsigned short family, port;
	unsigned address;
	unsigned long long padding;
};

struct SocketSet
{
	unsigned count;
	Socket sockets[64];
};

struct SocketTimeout
{
	long seconds, microseconds;
};

struct TimePacket
{
	unsigned header[4];
	unsigned long long timestamps[4];
};

template<class Char>
struct UrlComponents
{
	DWORD size;
	Char *scheme;
	DWORD schemeLength;
	int schemeId;
	Char *host;
	DWORD hostLength;
	unsigned short port;
	Char *user;
	DWORD userLength;
	Char *password;
	DWORD passwordLength;
	Char *path;
	DWORD pathLength;
	Char *extra;
	DWORD extraLength;
};

struct WinInetSlot
{
	unsigned char requestData[0x100];
	volatile long inUse;
	void *session;
	void *connection;
	unsigned char remaining[0x28];
};

static_assert(sizeof(UrlComponents<char>) == 0x68);
static_assert(sizeof(WinInetSlot) == 0x140 && __builtin_offsetof(WinInetSlot, session) == 0x108);
static_assert(sizeof(TimePacket) == 48 && sizeof(Ipv4Endpoint) == 16);
}

extern "C" __declspec(dllimport) unsigned short WINAPI htons(unsigned short);
extern "C" __declspec(dllimport) unsigned WINAPI htonl(unsigned);
extern "C" __declspec(dllimport) unsigned WINAPI inet_addr(const char *);
extern "C" __declspec(dllimport) Socket WINAPI socket(int, int, int);
extern "C" __declspec(dllimport) int WINAPI connect(Socket, const void *, int);
extern "C" __declspec(dllimport) int WINAPI closesocket(Socket);
extern "C" __declspec(dllimport) int WINAPI sendto(Socket, const void *, int, int, const void *, int);
extern "C" __declspec(dllimport) int WINAPI recvfrom(Socket, void *, int, int, void *, int *);
extern "C" __declspec(dllimport) int WINAPI select(int, SocketSet *, void *, void *, const SocketTimeout *);
extern "C" __declspec(dllimport) int WINAPI WSAGetLastError();
extern "C" long long _time64(long long *);
extern "C" unsigned long long t6_timeProbeEpoch;
extern "C" long long t6_timeProbeStart;
extern "C" unsigned char t6_winInetReady, t6_winInetStopping;
extern "C" WinInetSlot t6_winInetSlots[4];
extern "C" void WinInetReleaseConnection(void *);
extern "C" __declspec(dllimport) BOOL WINAPI InternetCrackUrlA(const char *, DWORD, DWORD, UrlComponents<char> *);
extern "C" __declspec(dllimport) void *WINAPI InternetOpenA(const char *, DWORD, const char *, const char *, DWORD);
extern "C" __declspec(dllimport) void *WINAPI InternetConnectA(void *, const char *, unsigned short, const char *,
                                                               const char *, DWORD, DWORD, __UINTPTR_TYPE__);

static void SwapTimePacket(TimePacket &packet)
{
	for (auto &word : packet.header)
		word = htonl(word);
	for (auto &timestamp : packet.timestamps)
	{
		const unsigned high = htonl(static_cast<unsigned>(timestamp >> 32));
		const unsigned low = htonl(static_cast<unsigned>(timestamp));
		timestamp = (static_cast<unsigned long long>(low) << 32) | high;
	}
}

extern "C" bool Sys_ProbeTimeServer(const char *address)
{
	if (!t6_timeProbeEpoch)
	{
		const auto seconds = _time64(nullptr);
		t6_timeProbeEpoch = (static_cast<unsigned long long>(seconds) - 0x7c558180u) << 32;
		t6_timeProbeStart = Sys_Milliseconds();
	}

	const Ipv4Endpoint destination{2, htons(123), inet_addr(address), 0};
	const int handle = static_cast<int>(socket(2, 2, 0));
	if (handle < 0)
		return false;
	const Socket handleAsSocket = static_cast<Socket>(handle);
	bool received = false;
	if (connect(handleAsSocket, &destination, sizeof(destination)) >= 0)
	{
		SocketSet readable;
		readable.count = 1;
		readable.sockets[0] = static_cast<unsigned>(handle);
		const SocketTimeout timeout{3, 0};
		for (int attempt = 0; attempt < 4; ++attempt)
		{
			TimePacket request{}, reply{};
			request.header[0] = 0x1b000000;
			const auto elapsed = static_cast<unsigned long long>(Sys_Milliseconds()) - t6_timeProbeStart;
			request.timestamps[3] = t6_timeProbeEpoch + (elapsed << 32) / 1000;
			SwapTimePacket(request);
			if (sendto(handleAsSocket, &request, sizeof(request), 0, nullptr, 0) != sizeof(request))
				break;
			if (select(static_cast<int>(static_cast<unsigned>(handle) + 1), &readable,
			           nullptr, nullptr, &timeout) != 1)
				continue;
			if (recvfrom(handleAsSocket, &reply, sizeof(reply), 0, nullptr, nullptr) == sizeof(reply))
			{
				Sys_Milliseconds();
				SwapTimePacket(reply);
				received = true;
			}
			break;
		}
	}
	closesocket(handleAsSocket);
	return received;
}

int BdConnectStreamSocket(int socket, const unsigned *ipv4, unsigned short port)
{
	const unsigned address = *ipv4;
	const Ipv4Endpoint destination{2, htons(port), address, 0};
	if (connect(static_cast<Socket>(socket), &destination, sizeof(destination)) >= 0)
		return 1;

	switch (WSAGetLastError())
	{
	case 10004: return -8;
	case 10022: return -9;
	case 10035: return -2;
	case 10040: return -6;
	case 10050: case 10051: case 10053: case 10054: case 10060: case 10065: return -5;
	default: return -1;
	}
}

extern "C" int BdConnectStream(int socket, const unsigned *ipv4, unsigned short port)
{
	return BdConnectStreamSocket(socket, ipv4, port);
}

extern "C" void *WinInetOpenConnection(const char *url)
{
	if (!t6_winInetReady || t6_winInetStopping)
		return nullptr;

	char host[256];
	char path[1024];
	UrlComponents<char> parts{};
	parts.size = sizeof(parts);
	parts.host = host;
	parts.hostLength = sizeof(host);
	parts.path = path;
	parts.pathLength = sizeof(path);
	if (!InternetCrackUrlA(url, static_cast<DWORD>(strlen(url)), 0, &parts))
		return nullptr;

	WinInetSlot *slot;
	for (unsigned index = 0;;)
	{
		slot = &t6_winInetSlots[index];
		long unused = 0;
		if (__atomic_compare_exchange_n(&slot->inUse, &unused, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
			break;
		if (++index == 4)
		{
			Sleep(10);
			index = 0;
		}
	}
	slot->session = InternetOpenA("codpcexe", 0, "", "", 0);
	if (slot->session)
	{
		slot->connection = InternetConnectA(slot->session, host, parts.port, nullptr, nullptr, 3, 0, 0);
		if (slot->connection)
			return slot;
	}
	WinInetReleaseConnection(slot);
	return nullptr;
}

extern "C" __declspec(dllimport) BOOL WINAPI WinHttpCrackUrl(const wchar_t *, DWORD, DWORD, UrlComponents<wchar_t> *);
extern "C" __declspec(dllimport) void *WINAPI WinHttpConnect(void *, const wchar_t *, unsigned short, DWORD);
WINBASEAPI DWORD WINAPI GetLastError();
extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" void *BdAllocate(__SIZE_TYPE__);
extern "C" void BdFree(void *);
extern "C" int BdFormatHttpHost(wchar_t *, const wchar_t *, ...);
extern "C" int BdFormatHttpPath(wchar_t *, __SIZE_TYPE__, const wchar_t *, ...);
extern "C" void BdCompleteWinHttpRequest(void *, unsigned);
extern "C" void BdSendWinHttpRequest(void *);

static unsigned HttpResult(unsigned error)
{
	return static_cast<int>(error) > 0 ? (error & 0xffffu) | 0x80070000u : error;
}

extern "C" void BdBeginWinHttpRequest(void *worker, void *request)
{
	constexpr char source[] =
		"d:\\workspace\\outrun\\t6\\pc\\dev\\code\\DemonWareGDK\\bdHTTP\\bdHTTPWorker\\bdHTTPWorkerWinHTTP.cpp";
	constexpr char function[] = "bdHTTPWorkerWinHTTPImpl::BeginHttpRequest";
	constexpr char category[] = "bdHTTPWorker(WinHTTP)";
	auto *workerBytes = static_cast<unsigned char *>(worker);
	auto *requestBytes = static_cast<unsigned char *>(request);
	auto &id = *reinterpret_cast<unsigned *>(requestBytes + 0x10);
	BdPlatformLog(0, "info/", category, source, function, 644, "HTTP Request[%04X]: BeginHttpRequest()", id);

	UrlComponents<wchar_t> parts{};
	parts.size = sizeof(parts);
	parts.hostLength = parts.pathLength = parts.extraLength = ~0u;
	if (!WinHttpCrackUrl(reinterpret_cast<const wchar_t *>(requestBytes + 0x88), 0, 0, &parts))
	{
		BdPlatformLog(2, "err/", category, source, function, 657,
		              "HTTP Request[%04X]: WinHttpCrackUrl() Failed with Error[%08X]", id, HttpResult(GetLastError()));
		BdCompleteWinHttpRequest(request, HttpResult(GetLastError()));
		return;
	}

	*reinterpret_cast<int *>(requestBytes + 0x10a8) = parts.schemeId;
	if (parts.schemeId != 2)
	{
		BdPlatformLog(2, "err/", category, source, function, 668,
		              "HTTP Request[%04X]: Unsupported scheme [%d]. Only HTTPS requests are supported", id, parts.schemeId);
		BdCompleteWinHttpRequest(request, 0x8007000du);
		return;
	}

	wchar_t host[256]{};
	BdFormatHttpHost(host, L"%.*s", parts.hostLength, parts.host);
	const unsigned capacity = parts.pathLength + parts.extraLength + 1;
	auto &path = *reinterpret_cast<wchar_t **>(requestBytes + 0x20c8);
	auto *newPath = static_cast<wchar_t *>(BdAllocate(static_cast<__SIZE_TYPE__>(capacity) * sizeof(wchar_t)));
	auto *oldPath = path;
	path = newPath;
	if (oldPath)
		BdFree(oldPath);
	BdFormatHttpPath(path, capacity, L"%.*s%.*s", parts.pathLength, parts.path, parts.extraLength, parts.extra);

	void *connection = WinHttpConnect(*reinterpret_cast<void **>(workerBytes + 0xa0), host, parts.port, 0);
	if (connection)
	{
		BdPlatformLog(0, "info/", category, source, function, 702,
		              "HTTP Request[%04X]: Created WinHTTP Connection[%X]", id, connection);
		*reinterpret_cast<void **>(requestBytes + 0xa0d0) = connection;
		BdSendWinHttpRequest(request);
		return;
	}
	BdPlatformLog(2, "err/", category, source, function, 696,
	              "HTTP Request[%04X]: WinHTTPConnect() Failed with Error[%08X]", id, HttpResult(GetLastError()));
	BdCompleteWinHttpRequest(request, HttpResult(GetLastError()));
}

extern "C" __declspec(dllimport) void *WINAPI WinHttpOpenRequest(void *, const wchar_t *, const wchar_t *,
                                                               const wchar_t *, const wchar_t *, const wchar_t **, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSetTimeouts(void *, int, int, int, int);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpCloseHandle(void *);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpAddRequestHeaders(void *, const wchar_t *, DWORD, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSendRequest(void *, const wchar_t *, DWORD, void *, DWORD, DWORD,
                                                               __UINTPTR_TYPE__);

extern "C" void BdSendWinHttpRequest(void *request)
{
	constexpr char source[] =
		R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdHTTP\bdHTTPWorker\bdHTTPWorkerWinHTTP.cpp)";
	constexpr char function[] = "bdHTTPWorkerWinHTTPImpl::SendHttpRequest";
	constexpr char category[] = "bdHTTPWorker(WinHTTP)";
	auto *bytes = static_cast<unsigned char *>(request);
	const auto &id = *reinterpret_cast<unsigned *>(bytes + 0x10);
	const auto fail = [&](int line, const char *message)
	{
		BdPlatformLog(2, "err/", category, source, function, line, message, id, HttpResult(GetLastError()));
	};

	BdPlatformLog(0, "info/", category, source, function, 711, "HTTP Request[%04X]: SendHttpRequest()", id);
	void *handle = WinHttpOpenRequest(*reinterpret_cast<void **>(bytes + 0xa0d0),
	                                reinterpret_cast<const wchar_t *>(bytes + 0x1088),
	                                *reinterpret_cast<const wchar_t **>(bytes + 0x20c8),
	                                nullptr, nullptr, nullptr, 0x800000);
	if (!handle)
		fail(733, "HTTP Request[%04X]: WinHttpOpenRequest() Failed with Error[%08X]");
	else
	{
		auto &storedHandle = *reinterpret_cast<void **>(bytes + 0xa0d8);
		storedHandle = handle;
		BdPlatformLog(0, "info/", category, source, function, 739,
		              "HTTP Request[%04X]: Created WinHTTP Request[%X]", id, handle);
		const auto &timeout = *reinterpret_cast<int *>(bytes + 0x20b0);
		const BOOL timeoutsSet = WinHttpSetTimeouts(storedHandle, 15000, 60000, timeout, timeout);
		BdPlatformLog(0, "info/", category, source, function, 783,
		              "HTTP Request[%04X]: Setting timeout to [%d]ms", id, timeout);
		const auto *headers = reinterpret_cast<const wchar_t *>(bytes + 0x20d0);
		if (!timeoutsSet)
		{
			fail(786, "HTTP Request[%04X]: WinHttpSetTimeouts() Failed with Error[%08X]");
			WinHttpCloseHandle(storedHandle);
			storedHandle = nullptr;
		}
		else if (*headers && !WinHttpAddRequestHeaders(handle, headers, ~0u, 0x20000000))
			fail(803, "HTTP Request[%04X]: Failed to add headers with Error[%08X]");
		else
		{
			void *body = *reinterpret_cast<void **>(bytes + 0x30);
			const auto length = *reinterpret_cast<unsigned *>(bytes + 0x38);
			const DWORD totalLength = !body && *reinterpret_cast<void **>(bytes + 0x48) ? length : 0;
			const BOOL sent = WinHttpSendRequest(handle, nullptr, 0, body, body ? length : 0, totalLength,
			                                    reinterpret_cast<__UINTPTR_TYPE__>(request));
			if (body)
				*reinterpret_cast<unsigned *>(bytes + 0x64) += length;
			if (sent)
				return;
			fail(851, "HTTP Request[%04X]: WinHttpSendRequest() Failed with Error[%08X]");
		}
	}
	BdCompleteWinHttpRequest(request, HttpResult(GetLastError()));
}
#endif
