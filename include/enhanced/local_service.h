// The local Demonware service the enhanced Store build runs in process (enhanced/demonware/local_service.cpp):
// Store auth answered directly, and the lobby protocol on a loopback TCP listener. This header is the protocol core,
// free of Windows, so tests/demonware can build it for the host; enhanced/demonware/local_service_win32.cpp supplies
// the platform functions below and the sockets and threads.
#pragma once

typedef unsigned char u8;
typedef unsigned long long u64;

namespace LocalService
{
constexpr u64 localUserID = 0x2100000000000001ull;  // the one local profile's Demonware user ID
constexpr unsigned frameLimit = 65535;              // the client's advertised lobby frame limit
constexpr unsigned ticketSize = 128;
constexpr unsigned keySize = 24;                    // Triple DES
constexpr unsigned maxPendingTickets = 128;
constexpr u64 ticketLifetimeMs = 60 * 1000;
constexpr unsigned maxProfileFiles = 128;
// The largest file one reply carries: five bytes before the cipher blocks and 37 before the blob's data.
constexpr unsigned maxStorageFile = (frameLimit - 5) / 8 * 8 - 37;

// Demonware result codes
enum : unsigned
{
	BD_NO_ERROR = 0,
	BD_HANDLE_TASK_FAILED = 4,
	BD_SERVICE_NOT_AVAILABLE = 108,
	BD_NO_FILE = 1000,
};

// ---- what the platform provides ----
bool TripleDesCbc(bool encrypt, const u8 key[keySize], const u8 iv[8], u8 *data, unsigned size);
bool HmacSha1(const u8 *key, unsigned keySize, const u8 *data, unsigned size, u8 out[20]);
void InitialVector(unsigned seed, u8 iv[24]);  // the client's own: bdCryptoUtils::calculateInitialVector
bool Random(u8 *out, unsigned size);
u64 UnixTime();
u64 MonotonicMs();
void Lock();  // one lock for the service's shared state (tickets, presence, profile files)
void Unlock();
void *Allocate(unsigned size);
void Free(void *p);
void Trace(const char *event);
// The profile's stored files (persisted by the platform): the file's size, or -1 when there is none.
int ReadProfileFile(const char *name, u8 *out, unsigned capacity);
bool WriteProfileFile(const char *name, const u8 *data, unsigned size);
// Publisher files from a folder of them, when there is one: as above; -2 when the file is too large or unreadable.
bool HasPublisherFolder();
int ReadPublisherFile(const char *name, u8 *out, unsigned capacity);

// ---- the core ----

// The Store auth reply for a sign-in (the JSON the game's parser reads, with the tickets the lobby will accept), or
// 0 when it can't be issued (too many sign-ins pending, or no randomness).
unsigned AnswerAuth(unsigned titleId, unsigned ivSeed, char *out, unsigned capacity);

struct Session
{
	u64 id;
	u8 key[keySize];
};

// The lobby handshake: the client's 8-byte hello and its auth frame (after the frame's length). True with the
// session and the 15-byte reply to send, false to close the connection.
bool Handshake(const u8 hello[8], const u8 *frame, unsigned size, Session &session, u8 reply[15]);

// One lobby frame (after its length) from an authenticated client: the bytes to send back (0 for none), or -1 to
// close the connection. `reply` holds frameLimit + 4 bytes.
int HandleFrame(Session &session, const u8 *frame, unsigned size, u8 *reply);
}  // namespace LocalService
