#pragma once

long GdkUserCheckPrivilege(void *user, unsigned options, unsigned privilege, bool *hasPrivilege, unsigned *reason);
long GdkUserCheckPrivilegeImpl(void *user, unsigned options, unsigned privilege, bool *hasPrivilege, unsigned *reason);

extern "C" long GdkLoadRuntime();
extern "C" long GdkInitializeRuntime(unsigned long long flags);
extern "C" long GdkQueryApi(const void *classId, const void *interfaceId, void **result);
extern "C" void GdkUninitializeRuntime();
extern "C" long GdkUserAddAsync(unsigned options, void *asyncBlock);
extern "C" long GdkUserAddResult(void *asyncBlock, void **user);
extern "C" long GdkUserDuplicateHandle(void *user, void **duplicate);
extern "C" long GdkUserGetId(void *user, unsigned long long *id);
extern "C" void GdkUserCloseHandle(void *user);
using GdkTaskQueueCallback = void (*)(void *context, bool cancelled);
extern "C" long GdkTaskQueueCreate(unsigned workMode, unsigned completionMode, void **queue);
extern "C" void GdkTaskQueueClose(void *queue);
extern "C" int HttpQueueCallback(void *queue, int port, void *context, GdkTaskQueueCallback callback);
extern "C" long GdkSavePath(void *context, __SIZE_TYPE__ capacity, char *path);
extern "C" int Sys_ReportUnhandledException(void *exception);
