#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include <windows.h>

WINBASEAPI void WINAPI OutputDebugStringA(const char *);

namespace
{
bool localDemonwareEnabled;
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

#endif
