#pragma once

// Address record used by the Store Demonware platform layer.
struct BdSocketAddress
{
	unsigned char address[128];
	unsigned short family;
	unsigned short reserved;
};

extern "C" unsigned BdGetHostByName(const char *name, BdSocketAddress *addresses, unsigned capacity);
extern "C" int BdSendTo(int socket, const BdSocketAddress *destination, unsigned short port,
                        const char *data, int size);
