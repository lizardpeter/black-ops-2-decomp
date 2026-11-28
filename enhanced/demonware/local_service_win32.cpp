// enhanced/demonware/local_service_win32.cpp: the local Demonware service's platform side (include/enhanced/
// local_service.h): Windows' CNG for Triple DES, HMAC-SHA1 and randomness (bcrypt.dll is one of the exe's imports),
// the client's own Tiger-based IV, the loopback TCP listener on the lobby port with one thread per connection, and
// the profile's files.
//
// The profile's files are kept in <game folder>\save\demonware\ (GdkSavePath's folder), one file each, written
// whole through a temporary file. A folder named publisher beside the exe, when there is one, serves the publisher
// files by name in place of the generated bundles.
#ifdef T6_STORE
#include "enhanced/local_demonware.h"
#include "enhanced/local_service.h"

#include <string.h>
#include <windows.h>

typedef void *BCRYPT_ALG_HANDLE;
typedef void *BCRYPT_HANDLE;
typedef long NTSTATUS;
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptOpenAlgorithmProvider(BCRYPT_ALG_HANDLE *, const wchar_t *,
                                                                               const wchar_t *, unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptSetProperty(BCRYPT_HANDLE, const wchar_t *, u8 *, unsigned long,
                                                                     unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptGenerateSymmetricKey(BCRYPT_ALG_HANDLE, BCRYPT_HANDLE *, u8 *,
                                                                              unsigned long, const u8 *, unsigned long,
                                                                              unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptEncrypt(BCRYPT_HANDLE, u8 *, unsigned long, void *, u8 *,
                                                                 unsigned long, u8 *, unsigned long, unsigned long *,
                                                                 unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptDecrypt(BCRYPT_HANDLE, u8 *, unsigned long, void *, u8 *,
                                                                 unsigned long, u8 *, unsigned long, unsigned long *,
                                                                 unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptDestroyKey(BCRYPT_HANDLE);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptCreateHash(BCRYPT_ALG_HANDLE, BCRYPT_HANDLE *, u8 *,
                                                                    unsigned long, const u8 *, unsigned long,
                                                                    unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptHashData(BCRYPT_HANDLE, const u8 *, unsigned long,
                                                                  unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptFinishHash(BCRYPT_HANDLE, u8 *, unsigned long, unsigned long);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptDestroyHash(BCRYPT_HANDLE);
extern "C" __declspec(dllimport) NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE, u8 *, unsigned long,
                                                                   unsigned long);

typedef unsigned long long SOCKET;
constexpr SOCKET invalidSocket = ~0ull;
struct sockaddr_in4
{
	unsigned short family;
	unsigned short port;  // network order
	unsigned address;     // network order
	u8 zero[8];
};
extern "C" __declspec(dllimport) SOCKET WINAPI socket(int, int, int);
extern "C" __declspec(dllimport) int WINAPI bind(SOCKET, const void *, int);
extern "C" __declspec(dllimport) int WINAPI listen(SOCKET, int);
extern "C" __declspec(dllimport) SOCKET WINAPI accept(SOCKET, void *, int *);
extern "C" __declspec(dllimport) int WINAPI recv(SOCKET, char *, int, int);
extern "C" __declspec(dllimport) int WINAPI send(SOCKET, const char *, int, int);
extern "C" __declspec(dllimport) int WINAPI setsockopt(SOCKET, int, int, const char *, int);
extern "C" __declspec(dllimport) int WINAPI closesocket(SOCKET);
extern "C" __declspec(dllimport) int WINAPI getsockname(SOCKET, void *, int *);

struct FileTime
{
	DWORD low, high;
};
constexpr DWORD genericRead = 0x80000000, openExisting = 3;
constexpr DWORD memCommit = 0x1000, memReserve = 0x2000, memRelease = 0x8000, pageReadWrite = 4;
WINBASEAPI BOOL WINAPI ReadFile(HANDLE, void *, DWORD, DWORD *, void *);
WINBASEAPI void *WINAPI VirtualAlloc(void *, __SIZE_TYPE__, DWORD, DWORD);
WINBASEAPI BOOL WINAPI VirtualFree(void *, __SIZE_TYPE__, DWORD);
WINBASEAPI void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION);
WINBASEAPI unsigned long long WINAPI GetTickCount64();
WINBASEAPI void WINAPI GetSystemTimeAsFileTime(FileTime *);
WINBASEAPI DWORD WINAPI GetFileAttributesW(const wchar_t *);
WINBASEAPI BOOL WINAPI CreateDirectoryW(const wchar_t *, void *);
WINBASEAPI DWORD WINAPI GetLastError();
WINBASEAPI BOOL WINAPI MoveFileExW(const wchar_t *, const wchar_t *, DWORD);
WINBASEAPI BOOL WINAPI GetFileSizeEx(HANDLE, LARGE_INTEGER *);
WINBASEAPI int WINAPI MultiByteToWideChar(UINT, DWORD, const char *, int, wchar_t *, int);
WINBASEAPI HANDLE WINAPI CreateThread(void *, __SIZE_TYPE__, DWORD(WINAPI *)(void *), void *, DWORD, DWORD *);
WINBASEAPI void WINAPI OutputDebugStringA(const char *);

// The client's own IV derivation: Tiger192 of the seed's four bytes (Store MP 14080E950, ZM 14080C490)
struct bdCryptoUtils
{
	static void calculateInitialVector(unsigned seed, unsigned char *iv);
};

extern "C" char t6_store_save_path[260];

namespace
{
CRITICAL_SECTION s_lock;
volatile long s_state;  // 0 not started, 1 starting, 2 started
BCRYPT_ALG_HANDLE s_des, s_hmac;
SOCKET s_listener = invalidSocket;
unsigned short s_lobbyPort;  // the listener's port (network order), 0 when it isn't listening
wchar_t s_profileDir[300], s_publisherDir[300];
bool s_publisherFolder;

bool Utf16(const char *in, wchar_t *out, int capacity)
{
	return MultiByteToWideChar(65001, 0, in, -1, out, capacity) > 0;
}

// dir + name (as a file name when escaped: characters other than letters, digits, '.', '-' and '_' written %XX)
bool FilePath(const wchar_t *dir, const char *name, wchar_t *out, unsigned capacity, const wchar_t *suffix = L"",
              bool escape = true)
{
	unsigned n = 0;
	for (; dir[n] && n < capacity; ++n)
		out[n] = dir[n];
	static const char hex[] = "0123456789ABCDEF";
	for (const unsigned char *c = reinterpret_cast<const unsigned char *>(name); *c; ++c)
	{
		const bool plain = (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9') ||
		                   *c == '-' || *c == '_' || (*c == '.' && c != reinterpret_cast<const unsigned char *>(name));
		if (n + 4 >= capacity)
			return false;
		if (plain || !escape)
			out[n++] = *c;
		else
			out[n++] = '%', out[n++] = hex[*c >> 4], out[n++] = hex[*c & 15];
	}
	for (; *suffix; ++suffix)
	{
		if (n + 1 >= capacity)
			return false;
		out[n++] = *suffix;
	}
	out[n] = 0;
	return true;
}

int ReadWholeFile(const wchar_t *path, u8 *out, unsigned capacity)
{
	const HANDLE file = CreateFileW(path, genericRead, FILE_SHARE_READ, nullptr, openExisting, FILE_ATTRIBUTE_NORMAL,
	                                nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return GetLastError() == 2 || GetLastError() == 3 ? -1 : -2;  // not found: no file
	LARGE_INTEGER size;
	int result = -2;
	if (GetFileSizeEx(file, &size) && size.QuadPart <= capacity)
	{
		DWORD read = 0;
		if (ReadFile(file, out, DWORD(size.QuadPart), &read, nullptr) && read == size.QuadPart)
			result = int(read);
	}
	CloseHandle(file);
	return result;
}

bool EnsureDirectory(const wchar_t *path)
{
	if (CreateDirectoryW(path, nullptr))
		return true;
	const DWORD attributes = GetFileAttributesW(path);
	return attributes != 0xFFFFFFFFu && (attributes & 0x10);
}

void Directories()
{
	// the profile's files: <save folder>\demonware\ (the folder is made on the first write)
	wchar_t save[260];
	if (t6_store_save_path[0] && Utf16(t6_store_save_path, save, 260))
		FilePath(save, "", s_profileDir, 300, L"\\demonware\\");
	// publisher files: <exe folder>\publisher\, when it exists
	wchar_t exe[260];
	DWORD n = GetModuleFileNameW(nullptr, exe, 260);
	while (n && exe[n - 1] != '\\')
		--n;
	if (n && n < 240)
	{
		exe[n] = 0;
		FilePath(exe, "", s_publisherDir, 300, L"publisher");
		const DWORD attributes = GetFileAttributesW(s_publisherDir);
		s_publisherFolder = attributes != 0xFFFFFFFFu && (attributes & 0x10);
		FilePath(exe, "", s_publisherDir, 300, L"publisher\\");
	}
}

bool ReadExact(SOCKET s, u8 *p, unsigned n)
{
	while (n)
	{
		const int got = recv(s, reinterpret_cast<char *>(p), int(n), 0);
		if (got <= 0)
			return false;
		p += got, n -= unsigned(got);
	}
	return true;
}

bool SendAll(SOCKET s, const u8 *p, unsigned n)
{
	while (n)
	{
		const int sent = send(s, reinterpret_cast<const char *>(p), int(n), 0);
		if (sent <= 0)
			return false;
		p += sent, n -= unsigned(sent);
	}
	return true;
}

// A frame's length and contents (the length at most the client's advertised limit).
bool ReadFrame(SOCKET s, u8 *frame, unsigned &size)
{
	u8 length[4];
	if (!ReadExact(s, length, 4))
		return false;
	size = length[0] | length[1] << 8 | length[2] << 16 | unsigned(length[3]) << 24;
	return size <= LocalService::frameLimit && ReadExact(s, frame, size);
}

void Timeouts(SOCKET s, DWORD receiveMs)
{
	constexpr int solSocket = 0xFFFF, soSndTimeo = 0x1005, soRcvTimeo = 0x1006;
	const DWORD sendMs = 5000;
	setsockopt(s, solSocket, soRcvTimeo, reinterpret_cast<const char *>(&receiveMs), sizeof(receiveMs));
	setsockopt(s, solSocket, soSndTimeo, reinterpret_cast<const char *>(&sendMs), sizeof(sendMs));
}

DWORD WINAPI Connection(void *argument)
{
	const SOCKET s = SOCKET(argument);
	u8 *frame = static_cast<u8 *>(LocalService::Allocate(2 * (LocalService::frameLimit + 8)));
	if (frame)
	{
		u8 *reply = frame + LocalService::frameLimit + 8;
		LocalService::Session session;
		u8 hello[8], ack[15];
		unsigned size;
		// five seconds for the handshake; then none, since the client pauses lobby traffic while a map loads
		Timeouts(s, 5000);
		if (ReadExact(s, hello, 8) && ReadFrame(s, frame, size) &&
		    LocalService::Handshake(hello, frame, size, session, ack) && SendAll(s, ack, sizeof(ack)))
		{
			LocalDemonware_Trace("T6 Demonware: lobby connection authenticated\n");
			Timeouts(s, 0);
			while (ReadFrame(s, frame, size))
			{
				const int n = LocalService::HandleFrame(session, frame, size, reply);
				if (n < 0 || (n > 0 && !SendAll(s, reply, unsigned(n))))
					break;
			}
		}
		LocalService::Free(frame);
	}
	closesocket(s);
	return 0;
}

DWORD WINAPI Listener(void *)
{
	for (;;)
	{
		const SOCKET client = accept(s_listener, nullptr, nullptr);
		if (client == invalidSocket)
			continue;
		const HANDLE thread = CreateThread(nullptr, 0, Connection, reinterpret_cast<void *>(client), 0, nullptr);
		if (thread)
			CloseHandle(thread);
		else
			closesocket(client);
	}
}
}  // namespace

void LocalDemonware_Start()
{
	if (s_state == 2 || _InterlockedCompareExchange(&s_state, 1, 0) != 0)
	{
		while (s_state != 2)
			Sleep(0);
		return;
	}
	InitializeCriticalSection(&s_lock);
	BCryptOpenAlgorithmProvider(&s_des, L"3DES", nullptr, 0);
	if (s_des)
		BCryptSetProperty(s_des, L"ChainingMode", reinterpret_cast<u8 *>(const_cast<wchar_t *>(L"ChainingModeCBC")),
		                  sizeof(L"ChainingModeCBC"), 0);
	constexpr unsigned long hmacFlag = 8;  // BCRYPT_ALG_HANDLE_HMAC_FLAG
	BCryptOpenAlgorithmProvider(&s_hmac, L"SHA1", nullptr, hmacFlag);
	Directories();

	// the lobby, on a loopback port of its own (the game still asks for 3074: BdConnectStream redirects it), so
	// nothing else on the machine using 3074 gets in the way
	s_listener = socket(2, 1, 6);  // AF_INET, SOCK_STREAM, IPPROTO_TCP
	sockaddr_in4 at = {2, 0, demonwareLoopback, {}};
	int size = sizeof(at);
	HANDLE thread = nullptr;
	if (s_listener != invalidSocket && !bind(s_listener, &at, sizeof(at)) && !listen(s_listener, 8) &&
	    !getsockname(s_listener, &at, &size))
		thread = CreateThread(nullptr, 0, Listener, nullptr, 0, nullptr);
	if (thread)
	{
		CloseHandle(thread);
		s_lobbyPort = at.port;
		LocalDemonware_Trace("T6 Demonware: lobby listening on loopback\n");
	}
	else
	{
		if (s_listener != invalidSocket)
			closesocket(s_listener);
		s_listener = invalidSocket;
		OutputDebugStringA("T6 Demonware: the lobby could not listen on loopback\n");
	}
	_InterlockedExchange(&s_state, 2);
}

unsigned short LocalDemonware_LobbyPort()
{
	LocalDemonware_Start();
	return s_lobbyPort;
}

namespace
{
// The native WinHTTP response object the auth pump reads (BdConstructWinHttpResponse)
struct AuthRequest
{
	unsigned char prefix[0x14];
	DWORD status;
	unsigned char beforeDownload[8];
	char *download;
	DWORD downloadCapacity;
	unsigned char padding[4];
	void *upload;
	DWORD uploadLength;
	unsigned char beforeDownloadLength[0x60 - 0x3c];
	DWORD downloadLength;
	DWORD sentLength;
};
static_assert(__builtin_offsetof(AuthRequest, download) == 0x20, "AuthRequest");
static_assert(__builtin_offsetof(AuthRequest, upload) == 0x30, "AuthRequest");
static_assert(__builtin_offsetof(AuthRequest, downloadLength) == 0x60, "AuthRequest");
}  // namespace

bool LocalDemonware_AnswerAuth(void *request, unsigned titleId)
{
	LocalDemonware_Start();
	auto &auth = *static_cast<AuthRequest *>(request);
	if (!auth.upload || !auth.uploadLength || !auth.download || auth.downloadCapacity < 2)
		return false;
	// the request is the one BdHandleGdkAuthTask formed ({"auth_task":44,"iv_seed":0,"title_id":<title>})
	const unsigned length = LocalService::AnswerAuth(titleId, 0, auth.download, auth.downloadCapacity);
	if (!length)
		return false;
	auth.status = 200;
	auth.sentLength = auth.uploadLength;
	auth.downloadLength = length;
	return true;
}

namespace LocalService
{
bool TripleDesCbc(bool encrypt, const u8 key[keySize], const u8 iv[8], u8 *data, unsigned size)
{
	BCRYPT_HANDLE k = nullptr;
	if (!s_des || BCryptGenerateSymmetricKey(s_des, &k, nullptr, 0, key, keySize, 0) < 0)
		return false;
	u8 chain[8];
	memcpy(chain, iv, 8);
	unsigned long done = 0;
	const NTSTATUS status = encrypt ? BCryptEncrypt(k, data, size, nullptr, chain, 8, data, size, &done, 0)
	                                : BCryptDecrypt(k, data, size, nullptr, chain, 8, data, size, &done, 0);
	BCryptDestroyKey(k);
	return status >= 0 && done == size;
}

bool HmacSha1(const u8 *key, unsigned keyLength, const u8 *data, unsigned size, u8 out[20])
{
	BCRYPT_HANDLE h = nullptr;
	if (!s_hmac || BCryptCreateHash(s_hmac, &h, nullptr, 0, key, keyLength, 0) < 0)
		return false;
	const bool ok = BCryptHashData(h, data, size, 0) >= 0 && BCryptFinishHash(h, out, 20, 0) >= 0;
	BCryptDestroyHash(h);
	return ok;
}

void InitialVector(unsigned seed, u8 iv[24]) { bdCryptoUtils::calculateInitialVector(seed, iv); }

bool Random(u8 *out, unsigned size)
{
	constexpr unsigned long systemRng = 2;  // BCRYPT_USE_SYSTEM_PREFERRED_RNG
	return BCryptGenRandom(nullptr, out, size, systemRng) >= 0;
}

u64 UnixTime()
{
	FileTime t;
	GetSystemTimeAsFileTime(&t);
	return ((u64(t.high) << 32 | t.low) - 116444736000000000ull) / 10000000;
}

u64 MonotonicMs() { return GetTickCount64(); }
void Lock() { EnterCriticalSection(&s_lock); }
void Unlock() { LeaveCriticalSection(&s_lock); }
void *Allocate(unsigned size) { return VirtualAlloc(nullptr, size, memCommit | memReserve, pageReadWrite); }
void Free(void *p)
{
	if (p)
		VirtualFree(p, 0, memRelease);
}
void Trace(const char *event) { LocalDemonware_Trace(event); }

int ReadProfileFile(const char *name, u8 *out, unsigned capacity)
{
	wchar_t path[600];
	if (!s_profileDir[0] || !FilePath(s_profileDir, name, path, 600))
		return -1;
	const int size = ReadWholeFile(path, out, capacity);
	return size < 0 ? -1 : size;
}

bool WriteProfileFile(const char *name, const u8 *data, unsigned size)
{
	wchar_t path[600], temporary[600];
	if (!s_profileDir[0] || !EnsureDirectory(s_profileDir) || !FilePath(s_profileDir, name, path, 600) ||
	    !FilePath(s_profileDir, name, temporary, 600, L".tmp"))
		return false;
	const HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
		return false;
	DWORD written = 0;
	const bool ok = (!size || WriteFile(file, data, size, &written, nullptr)) && written == size;
	CloseHandle(file);
	constexpr DWORD replaceExisting = 1, writeThrough = 8;
	return ok && MoveFileExW(temporary, path, replaceExisting | writeThrough);
}

bool HasPublisherFolder() { return s_publisherFolder; }

int ReadPublisherFile(const char *name, u8 *out, unsigned capacity)
{
	wchar_t path[600];
	// (the name is already a plain local file name: LocalFileName in local_service.cpp)
	if (!FilePath(s_publisherDir, name, path, 600, L"", false))
		return -1;
	return ReadWholeFile(path, out, capacity);
}
}  // namespace LocalService
#endif
