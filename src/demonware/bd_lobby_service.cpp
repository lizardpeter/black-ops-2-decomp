#ifdef T6_STORE
#include "demonware/bd_reference.h"
#include <string.h>

namespace
{
struct LobbyService
{
	unsigned char beforeConnection[0x408];
	BdReferencable *connection;
	unsigned char connectionReady;
	unsigned char beforeAuth[3];
	unsigned char auth[0xa0];
	unsigned char flags;
};

static_assert(__builtin_offsetof(LobbyService, auth) == 0x414);
static_assert(__builtin_offsetof(LobbyService, flags) == 0x4b4);
}

extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" void *BdAllocate(__SIZE_TYPE__);
extern "C" void BdLobbyDisconnectConnection(BdReferencable *);
extern "C" void BdLobbyResetService(LobbyService *);
extern "C" BdReferencable *BdConstructCommonAddr(void *, void *);
extern "C" BdReferencable *BdConstructLobbyConnection(void *, BdReferencable **, unsigned, unsigned, LobbyService *);
extern "C" bool BdStartLobbyConnection(BdReferencable *, const unsigned char *);
extern "C" void BdDestroyLobbyAddress(void *);

extern "C" bool BdLobbyServiceConnect(LobbyService *service, void *address, const unsigned char *auth,
                                      unsigned sendLimit, unsigned receiveLimit, unsigned char flags)
{
	bool connected = false;
	if (!auth)
	{
		BdPlatformLog(2, "err/", "lobby service",
		              R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdLobby\bdLobbyService.cpp)",
		              "bdLobbyService::connect", 94, "No authentication info specified");
	}
	else
	{
		if (service->connection)
		{
			BdLobbyDisconnectConnection(service->connection);
			BdLobbyResetService(service);
		}

		service->flags = flags;
		// The Store build copies a vector at a time from the supplied auth record.
		for (unsigned offset = 0; offset < sizeof(service->auth); offset += 16)
		{
			unsigned char field[16];
			memcpy(field, auth + offset, sizeof(field));
			memcpy(service->auth + offset, field, sizeof(field));
		}

		auto *storage = BdAllocate(0xb8);
		auto *commonAddress = storage ? BdConstructCommonAddr(storage, address) : nullptr;
		if (commonAddress)
			++commonAddress->count;

		BdReferencable *connection = nullptr;
		storage = BdAllocate(0xc0);
		if (storage)
		{
			auto *ownedAddress = commonAddress;
			if (ownedAddress)
				++ownedAddress->count;
			connection = BdConstructLobbyConnection(storage, &ownedAddress, sendLimit, receiveLimit, service);
		}

		if (service->connection)
			service->connection->release();
		service->connection = connection;
		if (connection)
			++connection->count;
		service->connectionReady = 0;
		connected = BdStartLobbyConnection(connection, service->auth);
		if (commonAddress)
			commonAddress->release();
	}
	BdDestroyLobbyAddress(address);
	return connected;
}
#endif
