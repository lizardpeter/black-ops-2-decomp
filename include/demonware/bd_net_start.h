#pragma once

#include "demonware/bd_platform_socket.h"

template <typename T> struct BdNetArray
{
	T *data;
	unsigned capacity;
	unsigned size;
};

struct BdNetStartParams
{
	bool online;
	unsigned short gamePort;
	void *socket;
	BdNetArray<void *> discoveryHosts;
	unsigned short discoveryPort;
	BdNetArray<BdSocketAddress> localAddresses;
	BdSocketAddress publicAddress;
	unsigned short publicPort;
	unsigned discoveryRetries;
	float discoveryTimeout;
	unsigned char routerConfig[0xa0];
	bool bindAny;
	unsigned localNatType;
	unsigned char upnpConfig[0x10];
};

extern "C" BdNetStartParams *BdInitNetStartParams(BdNetStartParams *params);
void BdInitNetStartParamsImpl(BdNetStartParams *params);
