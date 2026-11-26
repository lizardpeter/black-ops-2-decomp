#pragma once

struct XCurlRequest
{
	unsigned char flag;
	void *handleOwner;
};

extern "C" void HttpDestroyEasyRequest(void *);
extern "C" void HttpFree(void *);
extern "C" int XCurlResult(int);
