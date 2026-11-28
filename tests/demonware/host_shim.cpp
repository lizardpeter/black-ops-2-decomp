// tests/demonware: the local Demonware service's protocol core (enhanced/demonware/local_service.cpp) built for the
// host, with its platform functions supplied by test_local_service.py through these callbacks.
#include "enhanced/local_service.h"

#include <cstdlib>
#include <cstring>

extern "C"
{
typedef int (*DesFn)(int encrypt, const u8 *key, const u8 *iv, u8 *data, unsigned size);
typedef int (*HmacFn)(const u8 *key, unsigned keySize, const u8 *data, unsigned size, u8 *out);
typedef void (*IvFn)(unsigned seed, u8 *iv);
typedef int (*RandomFn)(u8 *out, unsigned size);
typedef unsigned long long (*ClockFn)(int monotonic);
typedef void (*TraceFn)(const char *event);
typedef int (*ReadFn)(int publisher, const char *name, u8 *out, unsigned capacity);
typedef int (*WriteFn)(const char *name, const u8 *data, unsigned size);
typedef int (*FolderFn)();
}

static DesFn s_des;
static HmacFn s_hmac;
static IvFn s_iv;
static RandomFn s_random;
static ClockFn s_clock;
static TraceFn s_trace;
static ReadFn s_read;
static WriteFn s_write;
static FolderFn s_folder;

extern "C" void Setup(DesFn des, HmacFn hmac, IvFn iv, RandomFn random, ClockFn clock, TraceFn trace, ReadFn read,
                      WriteFn write, FolderFn folder)
{
	s_des = des, s_hmac = hmac, s_iv = iv, s_random = random, s_clock = clock, s_trace = trace;
	s_read = read, s_write = write, s_folder = folder;
}

namespace LocalService
{
bool TripleDesCbc(bool encrypt, const u8 key[keySize], const u8 iv[8], u8 *data, unsigned size)
{
	return s_des(encrypt, key, iv, data, size);
}
bool HmacSha1(const u8 *key, unsigned n, const u8 *data, unsigned size, u8 out[20]) { return s_hmac(key, n, data, size, out); }
void InitialVector(unsigned seed, u8 iv[24]) { s_iv(seed, iv); }
bool Random(u8 *out, unsigned size) { return s_random(out, size); }
u64 UnixTime() { return s_clock(0); }
u64 MonotonicMs() { return s_clock(1); }
void Lock() {}
void Unlock() {}
void *Allocate(unsigned size) { return std::malloc(size); }
void Free(void *p) { std::free(p); }
void Trace(const char *event) { s_trace(event); }
int ReadProfileFile(const char *name, u8 *out, unsigned capacity) { return s_read(0, name, out, capacity); }
bool WriteProfileFile(const char *name, const u8 *data, unsigned size) { return s_write(name, data, size); }
bool HasPublisherFolder() { return s_folder(); }
int ReadPublisherFile(const char *name, u8 *out, unsigned capacity) { return s_read(1, name, out, capacity); }
}  // namespace LocalService

extern "C" unsigned TestAnswerAuth(unsigned title, unsigned seed, char *out, unsigned capacity)
{
	return LocalService::AnswerAuth(title, seed, out, capacity);
}

extern "C" int TestHandshake(const u8 *hello, const u8 *frame, unsigned size, LocalService::Session *session, u8 *reply)
{
	return LocalService::Handshake(hello, frame, size, *session, reply);
}

extern "C" int TestHandleFrame(LocalService::Session *session, const u8 *frame, unsigned size, u8 *reply)
{
	return LocalService::HandleFrame(*session, frame, size, reply);
}
