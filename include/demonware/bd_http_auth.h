#pragma once

#include "demonware/bd_reference.h"

struct BdGdkAsyncAuthTask
{
	using Destroy = void (*)(BdGdkAsyncAuthTask *, unsigned);
	Destroy *vtable;
	unsigned char beforeStatus[0x40 - 8];
	unsigned status;
};

// Store bdHTTPAuthService fields used by the GDK task and HTTP response pump.
struct BdHttpAuthService
{
	unsigned char prefix[0x20];
	unsigned titleId;
	unsigned status;
	unsigned char beforeAsyncTask[0x150 - 0x28];
	BdGdkAsyncAuthTask *asyncTask;
	unsigned asyncStep;
	void *tokenTaskResult;
	BdReferencable *httpResponse;
	char upload[0x1400];
	char download[0x1400];
	unsigned error;
};

static_assert(__builtin_offsetof(BdHttpAuthService, asyncTask) == 0x150);
static_assert(__builtin_offsetof(BdHttpAuthService, tokenTaskResult) == 0x160);
static_assert(__builtin_offsetof(BdHttpAuthService, upload) == 0x170);
static_assert(__builtin_offsetof(BdHttpAuthService, download) == 0x1570);
static_assert(__builtin_offsetof(BdHttpAuthService, error) == 0x2970);

void BdHandleGdkAuthTask(BdHttpAuthService *auth);
void BdHandleGdkAuthTaskImpl(BdHttpAuthService *auth);
