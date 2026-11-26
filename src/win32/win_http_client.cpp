#ifdef T6_STORE
#include "win32/win_vendor_network.h"
#include "win32/win_http_client.h"
#include "win32/win_gdk.h"
#include <string.h>
#include <windows.h>

namespace
{
struct RequestNode
{
	RequestNode *left, *parent, *right;
	unsigned char color, nil;
	__UINTPTR_TYPE__ key;
	XCurlRequest request;
};

struct RequestTree
{
	RequestNode *head;
	__SIZE_TYPE__ size;
};

struct InsertPosition
{
	RequestNode *parent;
	unsigned left;
};

template<class Char>
struct ClientString
{
	union
	{
		Char inlineText[16 / sizeof(Char)];
		Char *heapText;
	};
	__SIZE_TYPE__ length, capacity;

	static constexpr __SIZE_TYPE__ inlineCapacity = 16 / sizeof(Char) - 1;
	const Char *data() const
	{
		return capacity > inlineCapacity ? heapText : inlineText;
	}
	~ClientString()
	{
		if (capacity > inlineCapacity)
			HttpFree(heapText);
	}
};

struct ClientConnection
{
	unsigned char unknown[0x20];
	void *session;
	void *connection;
	void *requestHandle;
	void *request;
	unsigned char url[0x158];
	unsigned usesBody;
	void *body;
	void *bodyCursor;
};

static_assert(sizeof(RequestNode) == 0x38 && __builtin_offsetof(RequestNode, key) == 0x20);
static_assert(sizeof(ClientString<wchar_t>) == 0x20 && __builtin_offsetof(ClientConnection, body) == 0x1a0);
}

extern "C" __declspec(dllimport) int curl_multi_add_handle(void *, void *);
extern "C" int HttpMutexLock(void *);
extern "C" void HttpMutexUnlock(void *);
extern "C" [[noreturn]] void HttpThrowMutexError(int);
extern "C" [[noreturn]] void HttpThrowRuntimeError(int);
extern "C" [[noreturn]] void HttpThrowTreeSize();
extern "C" void *HttpAllocate(__SIZE_TYPE__);
extern "C" void HttpConstructBadAlloc(void *);
extern "C" [[noreturn]] void HttpThrowException(void *, const void *);
extern "C" const unsigned char httpBadAllocInfo;
extern "C" void *XCurlEasyHandle(void *);
extern "C" void XCurlInsertNode(RequestTree *, const InsertPosition *, RequestNode *);
extern "C" void XCurlProcessQueue(void *, bool);
extern "C" int XCurlPerform(void *);
extern "C" void XCurlFailAll(void *, int);
extern "C" void HttpClientLog(const void *, int, const char *, ...);
extern "C" const unsigned char httpClientCategory;

extern "C" int XCurlAddRequest(void *multi, void *request)
{
	auto *bytes = static_cast<unsigned char *>(multi);
	auto &incoming = *static_cast<XCurlRequest *>(request);
	void *mutex = bytes + 0x10;
	const int locked = HttpMutexLock(mutex);
	if (locked)
		HttpThrowMutexError(locked);

	const int curlCode = curl_multi_add_handle(*reinterpret_cast<void **>(bytes), XCurlEasyHandle(incoming.handleOwner));
	int result;
	if (curlCode)
	{
		HttpClientLog(&httpClientCategory, 1, "XCurlMulti::AddRequest: curl_multi_add_handle failed with CURLCode=%u",
		              curlCode);
		result = XCurlResult(curlCode);
		HttpMutexUnlock(mutex);
	}
	else
	{
		auto &tree = *reinterpret_cast<RequestTree *>(bytes + 0x60);
		const auto key = reinterpret_cast<__UINTPTR_TYPE__>(XCurlEasyHandle(incoming.handleOwner));
		RequestNode *match = tree.head;
		InsertPosition position{tree.head->parent, 0};
		for (auto *node = position.parent; !node->nil;)
		{
			position.parent = node;
			position.left = key <= node->key;
			if (position.left)
				match = node;
			node = position.left ? node->left : node->right;
		}
		if (match->nil || key < match->key)
		{
			if (tree.size == 0x492492492492492ull)
				HttpThrowTreeSize();
			auto *node = static_cast<RequestNode *>(HttpAllocate(sizeof(RequestNode)));
			if (!node)
			{
				alignas(8) unsigned char exception[24];
				HttpConstructBadAlloc(exception);
				HttpThrowException(exception, &httpBadAllocInfo);
			}
			node->key = key;
			node->request.flag = incoming.flag;
			node->request.handleOwner = incoming.handleOwner;
			incoming.handleOwner = nullptr;
			node->left = node->parent = node->right = tree.head;
			node->color = node->nil = 0;
			XCurlInsertNode(&tree, &position, node);
		}
		if (!mutex)
			HttpThrowRuntimeError(1);
		HttpMutexUnlock(mutex);
		result = HttpQueueCallback(*reinterpret_cast<void **>(bytes + 8), 0, multi, XCurlProcessQueue);
		if (result >= 0)
			result = 0;
	}
	if (incoming.handleOwner)
	{
		HttpDestroyEasyRequest(incoming.handleOwner);
		HttpFree(incoming.handleOwner);
	}
	return result;
}

extern "C" void XCurlProcessQueue(void *multi, bool cancelled)
{
	if (cancelled)
	{
		XCurlFailAll(multi, static_cast<int>(0x80004004u)); // E_ABORT
		return;
	}
	const int result = XCurlPerform(multi);
	if (result < 0)
	{
		HttpClientLog(&httpClientCategory, 1, "%hs (hr=0x%08x)",
		              "XCurlMulti::Perform failed. Failing all active requests.", result);
		XCurlFailAll(multi, result);
	}
}

extern "C" int HttpGetRequestMethodAndUrl(void *, const char **, const char **);
extern "C" void HttpAssignNarrowString(ClientString<char> *, const char *, __SIZE_TYPE__);
extern "C" void *HttpParseUrl(void *, const ClientString<char> *);
extern "C" void HttpMoveUrl(void *, void *);
extern "C" void HttpDestroyUrl(void *);
extern "C" const ClientString<char> *HttpUrlHost(const void *);
extern "C" unsigned short HttpUrlPort(const void *);
extern "C" bool HttpUrlSecure(const void *);
extern "C" ClientString<char> *HttpUrlResource(const void *, ClientString<char> *);
extern "C" void HttpToWideString(ClientString<wchar_t> *, const ClientString<char> *);
extern "C" void HttpWidenMethod(ClientString<wchar_t> *, const char *);
extern "C" int HttpGetTimeout(void *, int *);
extern "C" int HttpGetBody(void *, void **, void **, __SIZE_TYPE__ *);
extern "C" int HttpGetHeaderCount(void *, unsigned *);
extern "C" void HttpFormatHeaders(ClientString<wchar_t> *, void *);
extern "C" unsigned long long HttpRequestId(void *);

WINBASEAPI DWORD WINAPI GetLastError();
WINBASEAPI DWORD WINAPI GetCurrentThreadId();
extern "C" __declspec(dllimport) void *WINAPI WinHttpConnect(void *, const wchar_t *, unsigned short, DWORD);
extern "C" __declspec(dllimport) void *WINAPI WinHttpOpenRequest(void *, const wchar_t *, const wchar_t *,
                                                               const wchar_t *, const wchar_t *, const wchar_t **, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSetTimeouts(void *, int, int, int, int);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpSetOption(void *, DWORD, void *, DWORD);
extern "C" __declspec(dllimport) BOOL WINAPI WinHttpAddRequestHeaders(void *, const wchar_t *, DWORD, DWORD);

static int LogWinHttpError(void *request, int severity, const char *format)
{
	const DWORD error = GetLastError();
	const DWORD thread = GetCurrentThreadId();
	const auto id = HttpRequestId(request);
	HttpClientLog(&httpClientCategory, severity, format, id, thread, error);
	return static_cast<int>(error) > 0 ? static_cast<int>((error & 0xffffu) | 0x80070000u) : static_cast<int>(error);
}

extern "C" int WinHttpInitializeRequest(void *connection)
{
	auto &state = *static_cast<ClientConnection *>(connection);
	const char *method = nullptr, *url = nullptr;
	int status = HttpGetRequestMethodAndUrl(state.request, &method, &url);
	if (status < 0)
		return status;
	{
		ClientString<char> text{};
		text.capacity = text.inlineCapacity;
		HttpAssignNarrowString(&text, url, strlen(url));
		alignas(8) unsigned char parsed[240];
		HttpMoveUrl(state.url, HttpParseUrl(parsed, &text));
		HttpDestroyUrl(parsed);
	}

	auto *host = HttpUrlHost(state.url);
	unsigned short port;
	if (host->length && !HttpUrlPort(state.url))
		port = HttpUrlSecure(state.url) ? 443 : 80;
	else
		port = HttpUrlPort(state.url);

	ClientString<wchar_t> wideHost;
	HttpToWideString(&wideHost, HttpUrlHost(state.url));
	state.connection = WinHttpConnect(state.session, wideHost.data(), port, 0);
	if (!state.connection)
		return LogWinHttpError(state.request, 1, "WinHttpConnection [ID %llu] [TID %ul] WinHttpConnect errorcode %d");

	ClientString<wchar_t> resource;
	{
		ClientString<char> narrowResource;
		HttpToWideString(&resource, HttpUrlResource(state.url, &narrowResource));
	}
	ClientString<wchar_t> verb;
	HttpWidenMethod(&verb, method);
	const DWORD flags = HttpUrlSecure(state.url) ? 0x800040u : 0x40u;
	state.requestHandle = WinHttpOpenRequest(state.connection, verb.data(), resource.data(), nullptr, nullptr, nullptr, flags);
	if (!state.requestHandle)
		return LogWinHttpError(state.request, 1, "WinHttpConnection [ID %llu] [TID %ul] WinHttpOpenRequest errorcode %d");

	int timeout = 0;
	status = HttpGetTimeout(state.request, &timeout);
	if (status < 0)
		return status;
	const int ms = static_cast<int>(static_cast<unsigned>(timeout) * 1000);
	if (!WinHttpSetTimeouts(state.session, ms, ms, ms, ms))
		return LogWinHttpError(state.request, 1, "WinHttpConnection [ID %llu] [TID %ul] WinHttpSetTimeouts errorcode %d");

	if (!static_cast<const unsigned char *>(state.request)[200])
	{
		DWORD options = 0x1100;
		if (!WinHttpSetOption(state.requestHandle, 0x1f, &options, sizeof(options)))
			LogWinHttpError(state.request, 2, "WinHttpConnection [ID %llu] [TID %ul] WinHttpSetOption errorcode %d");
	}
	void *bodyData = nullptr;
	__SIZE_TYPE__ bodySize = 0;
	status = HttpGetBody(state.request, &bodyData, &state.body, &bodySize);
	if (status < 0)
		return status;
	state.usesBody = state.body != nullptr;
	state.bodyCursor = state.body;

	unsigned headerCount = 0;
	status = HttpGetHeaderCount(state.request, &headerCount);
	if (status < 0)
		return status;
	if (headerCount)
	{
		ClientString<wchar_t> headers;
		HttpFormatHeaders(&headers, static_cast<unsigned char *>(state.request) + 0x90);
		if (!WinHttpAddRequestHeaders(state.requestHandle, headers.data(), static_cast<DWORD>(headers.length), 0x20000000))
			return LogWinHttpError(state.request, 1, "WinHttpConnection [ID %llu] [TID %ul] WinHttpAddRequestHeaders errorcode %d");
	}
	if (HttpUrlSecure(state.url) && !WinHttpSetOption(state.requestHandle, 0x2f, nullptr, 0))
		return LogWinHttpError(state.request, 1, "WinHttpConnection [ID %llu] [TID %ul] WinHttpSetOption errorcode %d");
	return 0;
}
#endif
