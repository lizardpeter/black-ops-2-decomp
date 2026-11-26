#ifdef T6_STORE
#include "win32/win_vendor_network.h"
#include "win32/win_http_client.h"

extern "C" int XCurlAddRequest(void *, void *request)
{
	const int failure = XCurlResult(4); // CURLM_INTERNAL_ERROR
	auto &incoming = *static_cast<XCurlRequest *>(request);
	if (incoming.handleOwner)
	{
		HttpDestroyEasyRequest(incoming.handleOwner);
		HttpFree(incoming.handleOwner);
	}
	return failure;
}

extern "C" int WinHttpInitializeRequest(void *)
{
	return static_cast<int>(0x80072efdu); // ERROR_WINHTTP_CANNOT_CONNECT as an HRESULT
}
#endif
