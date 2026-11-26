#ifdef T6_STORE
#include "demonware/bd_platform_socket.h"
#include <windows.h>
#include <string.h>

static_assert(sizeof(BdSocketAddress) == 0x84, "Store address stride");

namespace
{
constexpr int ipv4 = 2;
constexpr int ipv6 = 23;
constexpr int datagram = 2;
constexpr int numericHost = 4;

struct AddressInfo
{
	int flags, family, type, protocol;
	__SIZE_TYPE__ length;
	char *name;
	void *address;
	AddressInfo *next;
};

struct SocketAddress
{
	unsigned short family, port;
	unsigned char address[4];
	unsigned char padding[8];
};

struct SocketAddressV6
{
	unsigned short family, port;
	unsigned flow;
	unsigned char address[16];
	unsigned char padding[104];
};

struct SocketBuffer
{
	DWORD size;
	const char *data;
};
}

extern "C" int isalpha(int);
extern "C" __declspec(dllimport) int WINAPI getaddrinfo(const char *, const char *, const AddressInfo *, AddressInfo **);
extern "C" __declspec(dllimport) void WINAPI freeaddrinfo(AddressInfo *);
extern "C" __declspec(dllimport) unsigned short WINAPI htons(unsigned short);
extern "C" __declspec(dllimport) int WINAPI sendto(__UINTPTR_TYPE__, const char *, int, int, const void *, int);
extern "C" __declspec(dllimport) int WINAPI WSAGetLastError();
extern "C" __declspec(dllimport) int WINAPI WSASendTo(__UINTPTR_TYPE__, const SocketBuffer *, DWORD, DWORD *, DWORD,
                                                     const void *, int, void *, void *);
extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" unsigned long long bdDatagramBytesSent;
extern "C" unsigned long long bdDatagramsSent;

static constexpr char socketSource[] =
	"d:\\workspace\\outrun\\t6\\pc\\dev\\code\\DemonWareGDK\\bdPlatform\\bdPlatformSocket\\bdPlatformSocket-xboxone.cpp";

extern "C" unsigned BdGetHostByName(const char *name, BdSocketAddress *addresses, unsigned capacity)
{
	AddressInfo hints{};
	hints.family = ipv4;
	hints.type = datagram;
	if (!isalpha(static_cast<signed char>(name[0])))
		hints.flags = numericHost;

	AddressInfo *results = nullptr;
	if (getaddrinfo(name, nullptr, &hints, &results) || !results)
	{
		const int error = WSAGetLastError();
		if (error == 11001)
			BdPlatformLog(1, "warn/", "bdPlatformSocket", socketSource, "bdPlatformSocket::getHostByName",
			              527, "Host Not Found [%s]", name);
		else
			BdPlatformLog(1, "warn/", "bdPlatformSocket", socketSource, "bdPlatformSocket::getHostByName",
			              531, "getHostByName: Error [%d]", error);
		return 0;
	}

	unsigned count = 0;
	for (const AddressInfo *item = results; item && count < capacity; item = item->next, ++count)
	{
		SocketAddress address{};
		memcpy(&address, item->address, item->length);
		memcpy(addresses[count].address, address.address, sizeof(address.address));
	}
	freeaddrinfo(results);
	return count;
}

static int SendError(int error, bool v6)
{
	switch (error)
	{
	case 10004: return -8;
	case 10022: case 10039: case 10047: case 10049: return -7;
	case 10035: return -2;
	case 10040: return -6;
	case 10050: case 10051: case 10053: case 10054: case 10060: case 10065:
		BdPlatformLog(0, "info/", "bdPlatformSocket", socketSource, "bdPlatformSocket::sendTo",
		              v6 ? 327 : 258, v6 ? "Send: Received connection reset: error[%d]"
		                                  : "send: Received connection reset: error[%d]", error);
		return -5;
	default: return -1;
	}
}

extern "C" int BdSendTo(int socket, const BdSocketAddress *destination, unsigned short port,
                        const char *data, int size)
{
	if (socket == -1)
		return -10;

	int sent;
	int overhead;
	const auto handle = static_cast<__UINTPTR_TYPE__>(socket);
	if (destination->family == ipv4)
	{
		SocketAddress address{};
		address.family = ipv4;
		memcpy(address.address, destination->address, sizeof(address.address));
		address.port = htons(port);
		sent = sendto(handle, data, size, 0, &address, sizeof(address));
		if (sent < 0)
			return SendError(WSAGetLastError(), false);
		overhead = 28;
	}
	else if (destination->family == ipv6)
	{
		SocketAddressV6 address{};
		address.family = ipv6;
		memcpy(address.address, destination->address, sizeof(address.address));
		address.port = htons(port);
		const SocketBuffer buffer{static_cast<DWORD>(size), data};
		DWORD written = 0;
		if (WSASendTo(handle, &buffer, 1, &written, 0, &address, sizeof(address), nullptr, nullptr) < 0)
			return SendError(WSAGetLastError(), true);
		sent = static_cast<int>(written);
		overhead = 48;
	}
	else
		return -1;

	bdDatagramBytesSent += static_cast<int>(static_cast<unsigned>(sent) + overhead);
	++bdDatagramsSent;
	return sent;
}
#endif
