#ifdef T6_STORE
#include "demonware/bd_net_start.h"
#include "enhanced/local_demonware.h"

extern "C" void BdParseSocketAddress(BdSocketAddress *, const char *);

extern "C" BdNetStartParams *BdInitNetStartParams(BdNetStartParams *params)
{
	BdInitNetStartParamsImpl(params);
	// The native public-address override keeps the normal UDP setup and
	// lets bdNet finish without asking a STUN server for its address.
	BdParseSocketAddress(&params->publicAddress, "127.0.0.1");
	params->publicPort = params->gamePort;
	params->upnpConfig[0] = 0;
	return params;
}
#endif
