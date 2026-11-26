#ifdef T6_STORE
#include "demonware/bd_http_auth.h"

extern "C" void *BdAllocate(__SIZE_TYPE__);
extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" bool BdGdkAuthTaskIsPending(void *);
extern "C" const char *const *BdGdkAuthTaskError(void *);
extern "C" const void *BdGdkAuthTaskUser(void *);
extern "C" const void *BdGdkAuthTaskToken(void *);
extern "C" void BdSetGdkAuthUser(void **, const void *);
extern "C" void *XCurlEasyHandle(void *);
extern "C" BdGdkAsyncAuthTask *BdConstructTokenTask(void *, void *);
extern "C" void BdCopyAuthString(void **, const void *);
extern "C" void BdDestroyAuthString(void **);
extern "C" int BdSendAuthRequest(BdHttpAuthService *, void **);

void BdHandleGdkAuthTask(BdHttpAuthService *auth)
{
	BdHandleGdkAuthTaskImpl(auth);
}

void BdHandleGdkAuthTaskImpl(BdHttpAuthService *auth)
{
	auto *task = auth->asyncTask;
	if (BdGdkAuthTaskIsPending(task))
		return;

	constexpr char source[] =
		R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdLobby\bdHTTPAuthService-xboxone.cpp)";
	constexpr char function[] = "bdHTTPAuthService::handleGdkAsyncTask";
	constexpr char category[] = "bdHTTPAuthService";
	if (task->status == 4)
	{
		BdPlatformLog(2, "err/", category, source, function, 64, "Failed to execute GDK Task: %s", *BdGdkAuthTaskError(task));
		auth->error = 200;
		return;
	}

	switch (auth->asyncStep)
	{
	case 0:
	{
		BdSetGdkAuthUser(&auth->tokenTaskResult, BdGdkAuthTaskUser(task));
		auto *user = XCurlEasyHandle(&auth->tokenTaskResult);
		auto *storage = BdAllocate(0x60);
		auth->asyncTask = storage ? BdConstructTokenTask(storage, user) : nullptr;
		task->vtable[0](task, 1);
		auth->asyncStep = 1;
		return;
	}
	case 1:
	{
		void *token;
		BdCopyAuthString(&token, BdGdkAuthTaskToken(task));
		auth->asyncTask = nullptr;
		auth->asyncStep = 2;
		BdSendAuthRequest(auth, &token);
		BdSetGdkAuthUser(&auth->tokenTaskResult, nullptr);
		BdDestroyAuthString(&token);
		return;
	}
	default:
		BdPlatformLog(2, "err/", category, source, function, 89, "Unexpected GDK sub task: #%d", auth->asyncStep);
		auth->error = 324;
		return;
	}
}
#endif
