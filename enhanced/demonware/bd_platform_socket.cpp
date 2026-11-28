#ifdef T6_STORE
#include "demonware/bd_platform_socket.h"
#include "enhanced/local_demonware.h"
#include <string.h>

extern "C" unsigned BdGetHostByName(const char *name, BdSocketAddress *addresses, unsigned capacity)
{
	if (!capacity)
		return 0;
	const bool auth = !strcmp(name, "auth3.prod.demonware.net") || !strcmp(name, "auth3.dev.demonware.net");
	const bool lobby = !strcmp(name, "lsg.2410.prod.demonware.net") || !strcmp(name, "lsg.2405.dev.demonware.net");
	if (!auth && !lobby)
		return 0;
	LocalDemonware_Start();
	// The native resolver writes the address bytes into the caller's initialized record.
	memcpy(addresses[0].address, &demonwareLoopback, sizeof(demonwareLoopback));
	LocalDemonware_Trace(auth ? "T6 Demonware: auth DNS returned loopback\n"
	                        : "T6 Demonware: lobby DNS returned loopback\n");
	return 1;
}

extern "C" int BdSendTo(int socket, const BdSocketAddress *destination, unsigned short, const char *, int)
{
	if (socket == -1)
		return -10;
	return destination->family == 2 || destination->family == 23 ? -5 : -1;
}
#endif
