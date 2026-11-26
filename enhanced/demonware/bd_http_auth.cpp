#ifdef T6_STORE
#include "demonware/bd_http_auth.h"
#include "enhanced/local_demonware.h"
#include <string.h>

extern "C" int BdFormatAuthRequest(char *, __SIZE_TYPE__, const char *, ...);
extern "C" void *BdAllocate(__SIZE_TYPE__);
extern "C" void BdInitHttpRequest(void *, int, const char *);
extern "C" void BdDestroyHttpRequest(void *);
extern "C" bool BdHttpSetUpload(void *, const char *, unsigned);
extern "C" bool BdHttpSetDownload(void *, char *, unsigned);
extern "C" BdReferencable *BdConstructWinHttpResponse(void *, void *);
extern "C" void BdCompleteWinHttpRequest(void *, unsigned);
extern "C" void BdSetGdkAuthUser(void **, const void *);

void BdHandleGdkAuthTask(BdHttpAuthService *auth)
{
	if (!LocalDemonware_IsEnabled())
	{
		BdHandleGdkAuthTaskImpl(auth);
		return;
	}

	auto *task = auth->asyncTask;
	auth->asyncTask = nullptr;
	if (task)
		task->vtable[0](task, 1);
	BdSetGdkAuthUser(&auth->tokenTaskResult, nullptr);
	auth->asyncStep = 2;
	if (auth->httpResponse)
	{
		auth->httpResponse->release();
		auth->httpResponse = nullptr;
	}

	memset(auth->download, 0, sizeof(auth->download));
	const int length = BdFormatAuthRequest(auth->upload, sizeof(auth->upload),
			"{\"auth_task\":44,\"iv_seed\":0,\"title_id\":%u}", auth->titleId);
	if (length <= 0 || static_cast<unsigned>(length) >= sizeof(auth->upload))
	{
		auth->error = 734;
		auth->status = 0;
		return;
	}

	// Form a native response for the original auth pump. The stock queue uses
	// XNetworking before opening HTTP, even for loopback URLs.
	alignas(8) unsigned char request[0x880];
	BdInitHttpRequest(request, 3, demonwareAuthUrl);
	if (BdHttpSetUpload(request, auth->upload, static_cast<unsigned>(length)) &&
	    BdHttpSetDownload(request, auth->download, sizeof(auth->download)))
	{
		if (auto *storage = BdAllocate(0xa0e8))
		{
			auth->httpResponse = BdConstructWinHttpResponse(storage, request);
			++auth->httpResponse->count;
		}
	}
	BdDestroyHttpRequest(request);
	if (!auth->httpResponse)
	{
		auth->error = 705;
		auth->status = 0;
		return;
	}
	const bool received = LocalDemonware_SendAuthRequest(auth->httpResponse);
	LocalDemonware_Trace(received ? "T6 Demonware: local auth HTTP response received\n"
	                             : "T6 Demonware: local auth HTTP request failed\n");
	BdCompleteWinHttpRequest(auth->httpResponse, received ? 0 : 0x80072efdu);
}
#endif
