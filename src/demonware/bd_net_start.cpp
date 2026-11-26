#ifdef T6_STORE
#include "demonware/bd_net_start.h"

extern "C" void BdInitAddress(void *);
extern "C" void BdInitSocketRouterConfig(void *);
extern "C" void BdInitUpnpConfig(void *);
extern "C" void BdInitStringFromCString(void **, const char *);
extern "C" void BdAppendDiscoveryHost(BdNetArray<void *> *, void **);
extern "C" void BdDestroyAuthString(void **);

static_assert(__builtin_offsetof(BdNetStartParams, publicAddress) == 0x38);
static_assert(__builtin_offsetof(BdNetStartParams, discoveryRetries) == 0xc0);
static_assert(__builtin_offsetof(BdNetStartParams, bindAny) == 0x168);
static_assert(__builtin_offsetof(BdNetStartParams, upnpConfig) == 0x170);

void BdInitNetStartParamsImpl(BdNetStartParams *params)
{
	params->online = false;
	params->gamePort = 3074;
	params->socket = nullptr;
	params->discoveryHosts = {};
	params->localAddresses = {};
	BdInitAddress(&params->publicAddress);
	params->discoveryRetries = 1;
	params->discoveryTimeout = 10.0f;
	BdInitSocketRouterConfig(params->routerConfig);
	params->bindAny = true;
	params->localNatType = 0;
	BdInitUpnpConfig(params->upnpConfig);
	constexpr const char *discoveryHosts[] = {
		"stun.us.demonware.net", "stun.eu.demonware.net", "stun.jp.demonware.net", "stun.au.demonware.net",
	};
	for (const char *host : discoveryHosts)
	{
		void *value;
		BdInitStringFromCString(&value, host);
		BdAppendDiscoveryHost(&params->discoveryHosts, &value);
		BdDestroyAuthString(&value);
	}
	params->discoveryPort = 3074;
}

extern "C" BdNetStartParams *BdInitNetStartParams(BdNetStartParams *params)
{
	BdInitNetStartParamsImpl(params);
	return params;
}
#endif
