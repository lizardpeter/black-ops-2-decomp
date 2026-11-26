#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include "win32/win_vendor_network.h"
#include "win32/win_http_client.h"

struct XCurlProviderResult
{
	long status;
	unsigned long long inlineMessage[2];
	unsigned long long messageSize;
	unsigned long long messageCapacity;
	unsigned long long providerMetadata;
	void *provider;
	bool hasProvider;
};

static_assert(__builtin_offsetof(XCurlProviderResult, inlineMessage) == 0x08);
static_assert(__builtin_offsetof(XCurlProviderResult, hasProvider) == 0x38);

extern "C" XCurlProviderResult *XCurlCreateProvider(XCurlProviderResult *result)
{
	// Match the original curl_global_init failure result before XCurl can probe Gaming Services.
	result->status = static_cast<long>(0x80004005u); // E_FAIL
	result->inlineMessage[0] = 0;
	result->messageSize = 0;
	result->messageCapacity = 15;
	result->hasProvider = false;
	IdentityAudit_Trace("XCurl provider initialization rejected locally\n");
	return result;
}

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
