#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include "win32/win_vendor_network.h"

extern "C" bool Sys_ProbeTimeServer(const char *)
{
	return false;
}

extern "C" int BdConnectStream(int socket, const unsigned *ipv4, unsigned short port)
{
	if (!LocalDemonware_IsEnabled() || port != demonwareLobbyPort || *ipv4 != demonwareLoopback)
		return -5;
	LocalDemonware_Trace("T6 Demonware: opening loopback lobby TCP stream\n");
	return BdConnectStreamSocket(socket, ipv4, port);
}

extern "C" void *WinInetOpenConnection(const char *)
{
	return nullptr;
}

extern "C" void BdCompleteWinHttpRequest(void *, unsigned);

extern "C" void BdBeginWinHttpRequest(void *, void *request)
{
	BdCompleteWinHttpRequest(request, 0x80072efdu); // ERROR_WINHTTP_CANNOT_CONNECT as an HRESULT
}
#endif
