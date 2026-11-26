#ifdef T6_STORE
#include "demonware/bd_reference.h"
namespace
{
struct WinHttpResponse
{
	unsigned char prefix[0x10];
	unsigned requestId;
	unsigned httpStatus;
	unsigned char error;
	unsigned char beforeReferences[0x40 - 0x19];
	void *bodyReference;
	void *completionReference;
	unsigned char beforeTimer[0x68 - 0x50];
	unsigned long long startTick;
	float elapsed;
	unsigned char completionSignaled;
	unsigned signal;
	int status;
};

static_assert(__builtin_offsetof(WinHttpResponse, startTick) == 0x68);
static_assert(__builtin_offsetof(WinHttpResponse, signal) == 0x78);
static_assert(__builtin_offsetof(WinHttpResponse, status) == 0x7c);

struct BdReference
{
	void *vtable;
	BdReferencable *object;
};
}

extern "C" void BdPlatformLog(int, const char *, const char *, const char *, const char *, int, const char *, ...);
extern "C" long long BdPerformanceFrequency();
extern "C" long long BdPerformanceCounter();

extern "C" unsigned long long BdClockMicroseconds()
{
	const auto frequency = BdPerformanceFrequency();
	const auto counter = BdPerformanceCounter();
	const auto nanoseconds = frequency == 10000000 ? counter * 100 :
		(counter / frequency) * 1000000000 + ((counter % frequency) * 1000000000) / frequency;
	return static_cast<unsigned long long>(nanoseconds / 1000);
}

extern "C" float BdElapsedSeconds(unsigned long long start, unsigned long long end)
{
	if (end < start)
		return 0.0f;
	const auto difference = end - start;
	return static_cast<float>(difference) / 1000000.0f;
}

extern "C" float BdStopwatchElapsed(const unsigned long long *start)
{
	return *start ? BdElapsedSeconds(*start, BdClockMicroseconds()) : 0.0f;
}

extern "C" const char *BdHttpErrorName(unsigned char error)
{
	switch (error)
	{
	case 0: return "BD_SUCCESS";
	case 1: return "BD_WRITE_ERROR";
	case 2: return "BD_TIMEOUT";
	case 3: return "BD_ABORTED";
	case 4: return "BD_INIT_ERROR";
	case 5: return "BD_WORKER_ERROR";
	case 6: return "BD_UNKNOWN";
	case 7: return "BD_DNS_LOOKUP_ERROR";
	default: return "ERROR";
	}
}

extern "C" const char *BdHttpStatusName(int status)
{
	switch (status)
	{
	case 0: return "BD_READY";
	case 1: return "BD_PENDING";
	case 2: return "BD_FAILED";
	case 3: return "BD_DONE";
	default: return "ERROR";
	}
}

extern "C" void BdReleaseReference(void *reference)
{
	auto &object = static_cast<BdReference *>(reference)->object;
	if (object)
		object->release();
	object = nullptr;
}

extern "C" void BdCompleteWinHttpRequest(void *request, unsigned result)
{
	auto &response = *static_cast<WinHttpResponse *>(request);
	constexpr char source[] =
		R"(d:\workspace\outrun\t6\pc\dev\code\DemonWareGDK\bdHTTP\bdHTTPResponse\bdHTTPResponseWinHTTP.cpp)";
	constexpr char category[] = "bdHTTPResponseWinHTTP";
	constexpr char function[] = "bdHTTPResponseWinHTTP::finalize";
	if (static_cast<int>(result) < 0 && !response.error)
		response.error = result == 0x80004004u ? 3 : 6; // E_ABORT

	const float elapsed = BdStopwatchElapsed(&response.startTick);
	response.elapsed = elapsed < 0.01f ? 0.01f : elapsed;
	if (!response.error)
	{
		__atomic_exchange_n(&response.status, 3, __ATOMIC_SEQ_CST);
		BdPlatformLog(0, "info/", category, source, function, 349,
		              "HTTP Request[%04X]: Finalized Status[BD_DONE] HTTP Code[%u]", response.requestId, response.httpStatus);
	}
	else
	{
		__atomic_exchange_n(&response.status, 2, __ATOMIC_SEQ_CST);
		if (response.error == 3)
			BdPlatformLog(1, "warn/", category, source, function, 357,
			              "HTTP Request[%04X]: Finalized Status[BD_FAILED] HTTP Code[%u] InternalError[BD_ABORTED] Microsoft Code[%08X]",
			              response.requestId, response.httpStatus, result);
		else
			BdPlatformLog(2, "err/", category, source, function, 362,
			              "HTTP Request[%04X]: Finalized Status[BD_FAILED] HTTP Code[%u] InternalError[%s] Microsoft Code[%08X]",
			              response.requestId, response.httpStatus, BdHttpErrorName(response.error), result);
	}
	if (response.completionReference)
	{
		response.signal = 0;
		response.completionSignaled = 1;
		response.completionReference = nullptr;
	}
	if (response.bodyReference)
	{
		BdReleaseReference(response.bodyReference);
		response.bodyReference = nullptr;
	}
}
#endif
