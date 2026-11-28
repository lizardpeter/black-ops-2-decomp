// enhanced/demonware/local_service.cpp: the local Demonware service's protocol (include/enhanced/local_service.h).
//
// What the Store client sees, and what tests/demonware checks against the standalone service it replaced
// (services/demonware, kept as the base for a future online server):
//
// - Auth: a sign-in gets the Store auth reply {"auth_task":45,"code":700,...} with two 128-byte tickets: the client
//   ticket the game's parser reads (magic DEADBDEF, title, user ID, "Local Player", a fresh 24-byte session key) and an
//   opaque server ticket the game hands to the lobby. Each server ticket is accepted once, within a minute, for its
//   title, and brings its session key with it.
// - Lobby handshake: the client's hello (C8 00 00 00 FF FF 00 00), then a frame packing its title and server ticket
//   least significant bit first; the reply is the connection ID.
// - Lobby tasks: frames of a seed and Triple DES CBC data (the IV is the client's Tiger-based one for the seed),
//   checked with a truncated HMAC-SHA1. Each task gets one reply, in order: an empty frame a keepalive.
// - Tasks answered: the console report (0x26/4, acknowledged and dropped), rich presence (0x44/1 set, 0x44/2 query),
//   league teams and members (0x51/2, 6, 8: none), the profile's files (0x0A/1 upload, 0x0A/3 get, kept by the
//   platform), publisher files (0x0A/7: a folder of them when there is one, else generated playlist and league
//   bundles), server time (0x0C/6), region (0x1B/3: "Local"). Everything else, the bandwidth test included, gets
//   BD_SERVICE_NOT_AVAILABLE.
// (the Store builds, and tests/demonware's host build)
#if defined(T6_STORE) || defined(T6_HOST_TEST)
#include "enhanced/local_service.h"

#include <string.h>

namespace LocalService
{
namespace
{
// ---- little helpers over a byte buffer ----
struct Out
{
	u8 *data;
	unsigned size, capacity;
	bool ok = true;

	void Bytes(const void *p, unsigned n)
	{
		if (size + n > capacity)
		{
			ok = false;
			return;
		}
		memcpy(data + size, p, n);
		size += n;
	}
	void Byte(u8 b) { Bytes(&b, 1); }
	void U32(unsigned v)
	{
		const u8 b[4] = {u8(v), u8(v >> 8), u8(v >> 16), u8(v >> 24)};
		Bytes(b, 4);
	}
	void U64(u64 v)
	{
		U32(unsigned(v));
		U32(unsigned(v >> 32));
	}
	void U32BE(unsigned v)
	{
		const u8 b[4] = {u8(v >> 24), u8(v >> 16), u8(v >> 8), u8(v)};
		Bytes(b, 4);
	}
	void Text(const char *s) { Bytes(s, unsigned(strlen(s))); }
};

unsigned U32(const u8 *p) { return p[0] | p[1] << 8 | p[2] << 16 | unsigned(p[3]) << 24; }
u64 U64(const u8 *p) { return U32(p) | u64(U32(p + 4)) << 32; }

// ---- tickets ----
struct Pending
{
	bool used;
	u8 ticket[ticketSize];
	unsigned title;
	u8 key[keySize];
	u64 expires;
};
Pending s_pending[maxPendingTickets];

bool AddTicket(const u8 *ticket, unsigned title, const u8 *key, u64 now)
{
	Lock();
	unsigned count = 0;
	Pending *free = nullptr;
	for (Pending &p : s_pending)
	{
		if (p.used && now >= p.expires)
			p.used = false;
		if (p.used)
			++count;
		else if (!free)
			free = &p;
	}
	const bool added = count < maxPendingTickets && free;
	if (added)
	{
		free->used = true;
		memcpy(free->ticket, ticket, ticketSize);
		free->title = title;
		memcpy(free->key, key, keySize);
		free->expires = now + ticketLifetimeMs;
	}
	Unlock();
	return added;
}

bool ConsumeTicket(const u8 *ticket, unsigned title, u64 now, u8 key[keySize])
{
	Lock();
	bool found = false;
	for (Pending &p : s_pending)
	{
		if (!p.used || memcmp(p.ticket, ticket, ticketSize))
			continue;
		if (p.title == title && now < p.expires)
		{
			memcpy(key, p.key, keySize);
			p.used = false;
			found = true;
		}
		break;
	}
	Unlock();
	return found;
}

void Base64(Out &out, const u8 *data, unsigned size)
{
	static const char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	for (unsigned i = 0; i < size; i += 3)
	{
		const unsigned n = data[i] << 16 | (i + 1 < size ? data[i + 1] << 8 : 0) | (i + 2 < size ? data[i + 2] : 0);
		out.Byte(digits[n >> 18 & 63]);
		out.Byte(digits[n >> 12 & 63]);
		out.Byte(i + 1 < size ? digits[n >> 6 & 63] : '=');
		out.Byte(i + 2 < size ? digits[n & 63] : '=');
	}
}

// ---- tasks ----
u64 s_nextConnection, s_nextTransaction;  // under Lock()
u8 s_presence[32];
unsigned s_presenceSize;

struct Reply
{
	unsigned status;
	Out results;  // the results, back to back
	unsigned count;
};

unsigned UpdatePresence(const u8 *p, unsigned n)
{
	// setInfo writes a typed uint64 placeholder, then a typed blob
	if (n < 15 || p[0] != 10 || U64(p + 1) != 0 || p[9] != 19 || p[10] != 8)
		return BD_HANDLE_TASK_FAILED;
	const unsigned size = U32(p + 11);
	if (size > sizeof(s_presence) || size > n - 15)
		return BD_HANDLE_TASK_FAILED;
	Lock();
	memcpy(s_presence, p + 15, size);
	s_presenceSize = size;
	Unlock();
	return BD_NO_ERROR;
}

void QueryPresence(const u8 *p, unsigned n, Reply &r)
{
	constexpr unsigned maxUsers = 64;
	if (n < 9 || n / 9 > maxUsers || n % 9 == 8)
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	Lock();
	for (; n >= 9; p += 9, n -= 9)
	{
		if (p[0] != 10)
		{
			r.status = BD_HANDLE_TASK_FAILED;
			break;
		}
		// bdRichPresenceInfo: a typed boolean (online) and a typed blob
		const bool local = U64(p + 1) == localUserID;
		r.results.Byte(1);
		r.results.Byte(local);
		r.results.Byte(19);
		r.results.Byte(8);
		r.results.U32(local ? s_presenceSize : 0);
		if (local)
			r.results.Bytes(s_presence, s_presenceSize);
		++r.count;
	}
	Unlock();
	if (r.status)
		r.count = 0;
}

// A storage request's file name ("\x10" name "\0", under 128 characters) and what follows it.
bool StorageName(const u8 *p, unsigned n, char name[128], const u8 *&rest, unsigned &restSize)
{
	if (n < 3 || p[0] != 16)
		return false;
	const u8 *end = p + 1;
	while (end < p + n && *end)
		++end;
	if (end == p + n || end == p + 1 || end - (p + 1) >= 128)
		return false;
	memcpy(name, p + 1, end - (p + 1));
	name[end - (p + 1)] = 0;
	rest = end + 1;
	restSize = unsigned(p + n - rest);
	return true;
}

void FileResult(Reply &r, const u8 *data, unsigned size)
{
	r.results.Byte(19);
	r.results.Byte(8);
	r.results.U32(size);
	r.results.Bytes(data, size);
	r.count = 1;
}

// The profile files this session has written or read: the cap on how many there may be.
char s_profileNames[maxProfileFiles][128];
unsigned s_profileCount;

bool KnownProfileFile(const char *name)
{
	for (unsigned i = 0; i < s_profileCount; ++i)
		if (!strcmp(s_profileNames[i], name))
			return true;
	return false;
}

bool OwnerIsLocal(const u8 *owner, unsigned n) { return n >= 9 && n <= 16 && owner[0] == 10 && U64(owner + 1) == localUserID; }

void GetProfileFile(const u8 *p, unsigned n, Reply &r)
{
	char name[128];
	const u8 *owner;
	unsigned ownerSize;
	if (!StorageName(p, n, name, owner, ownerSize) || ownerSize < 9 || ownerSize > 16 || owner[0] != 10)
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	if (U64(owner + 1) != localUserID)
	{
		r.status = BD_NO_FILE;
		return;
	}
	u8 *data = static_cast<u8 *>(Allocate(maxStorageFile));
	if (!data)
	{
		r.status = BD_SERVICE_NOT_AVAILABLE;
		return;
	}
	Lock();
	const int size = ReadProfileFile(name, data, maxStorageFile);
	if (size >= 0 && !KnownProfileFile(name) && s_profileCount < maxProfileFiles)
		strcpy(s_profileNames[s_profileCount++], name);
	Unlock();
	if (size < 0)
		r.status = BD_NO_FILE;
	else
		FileResult(r, data, unsigned(size));
	Free(data);
}

void UploadProfileFile(const u8 *p, unsigned n, Reply &r)
{
	char name[128];
	const u8 *body;
	unsigned bodySize;
	// uploadFile writes a typed visibility flag, then a typed blob
	if (!StorageName(p, n, name, body, bodySize) || bodySize < 8 || body[0] != 1 || body[1] > 1 || body[2] != 19 ||
	    body[3] != 8)
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	const unsigned size = U32(body + 4);
	if (size > maxStorageFile || size > bodySize - 8)
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	// Up to seven trailing bytes are cipher padding; an owner, when given, must be the local profile
	const unsigned ownerSize = bodySize - 8 - size;
	if (ownerSize > 7 && !OwnerIsLocal(body + 8 + size, ownerSize))
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	Lock();
	const bool known = KnownProfileFile(name);
	if (!known && s_profileCount >= maxProfileFiles)
		r.status = BD_SERVICE_NOT_AVAILABLE;
	else if (!WriteProfileFile(name, body + 8, size))
		r.status = BD_SERVICE_NOT_AVAILABLE;
	else if (!known)
		strcpy(s_profileNames[s_profileCount++], name);
	Unlock();
}

// zlib's stored (uncompressed) form, which any inflater reads
void Zlib(Out &out, const char *text)
{
	const unsigned n = unsigned(strlen(text));
	out.Byte(0x78);
	out.Byte(0x01);
	out.Byte(1);  // the final block, stored
	out.Byte(u8(n));
	out.Byte(u8(n >> 8));
	out.Byte(u8(~n));
	out.Byte(u8(~n >> 8));
	out.Bytes(text, n);
	unsigned a = 1, b = 0;
	for (unsigned i = 0; i < n; ++i)
	{
		a = (a + u8(text[i])) % 65521;
		b = (b + a) % 65521;
	}
	out.U32BE(b << 16 | a);
}

// A publisher bundle as the Store client's online-file unpacker reads it: a header (magic 543377AB, the entry count,
// the publisher version -1 the client's no-update fallback also uses), 44-byte entries (name, then big-endian
// compressed size, size and offset), then each entry's zlib data.
unsigned Bundle(u8 *buffer, unsigned capacity, const char *playlist)
{
	const char *const names[] = {playlist, "leagues.info"};
	const char *const texts[] = {"version 1\n",
	                             "{\"version\":1,\"leagues\":[{\"name\":\"Local\"}],\"divisions\":[{\"name\":\"Local\"}]}"};
	constexpr unsigned headerSize = 16, entrySize = 44, entries = 2;
	if (capacity < headerSize + entries * entrySize)
		return 0;
	memset(buffer, 0, headerSize + entries * entrySize);
	Out out{buffer, headerSize + entries * entrySize, capacity};
	const u8 header[16] = {0xAB, 0x77, 0x33, 0x54, 0, 0, 0, 0, entries, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF};
	memcpy(buffer, header, sizeof(header));
	for (unsigned i = 0; i < entries; ++i)
	{
		const unsigned offset = out.size;
		Zlib(out, texts[i]);
		u8 *e = buffer + headerSize + i * entrySize;
		memcpy(e, names[i], strlen(names[i]));
		Out fields{e + 32, 0, 12};
		fields.U32BE(out.size - offset);
		fields.U32BE(unsigned(strlen(texts[i])));
		fields.U32BE(offset);
	}
	return out.ok ? out.size : 0;
}

// A name the folder of publisher files may serve: a plain file name (filepath.IsLocal with no separators).
bool LocalFileName(const char *name)
{
	if (!*name || !strcmp(name, ".") || !strcmp(name, ".."))
		return false;
	for (const char *c = name; *c; ++c)
		if (*c == '/' || *c == '\\' || *c == ':')
			return false;
	// Windows' reserved device names (with or without an extension)
	static const char *const reserved[] = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5",
	                                       "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5",
	                                       "LPT6", "LPT7", "LPT8", "LPT9"};
	for (const char *r : reserved)
	{
		unsigned i = 0;
		for (; r[i] && (name[i] & ~0x20) == r[i]; ++i)
		{
		}
		if (!r[i] && (!name[i] || name[i] == '.' || name[i] == ' '))
			return false;
	}
	return true;
}

void GetPublisherFile(const u8 *p, unsigned n, Reply &r)
{
	char name[128];
	const u8 *rest;
	unsigned restSize;
	if (!StorageName(p, n, name, rest, restSize))
	{
		r.status = BD_HANDLE_TASK_FAILED;
		return;
	}
	u8 *data = static_cast<u8 *>(Allocate(maxStorageFile + 1));
	if (!data)
	{
		r.status = BD_SERVICE_NOT_AVAILABLE;
		return;
	}
	if (!HasPublisherFolder())
	{
		// the generated bundles stand in for the publisher's own
		const char *playlist = !strcmp(name, "online_tu17_mp_english.wad")   ? "playlists_mp.info"
		                       : !strcmp(name, "online_tu17_zm_english.wad") ? "playlists_zm.info"
		                                                                     : nullptr;
		if (playlist)
			FileResult(r, data, Bundle(data, maxStorageFile, playlist));
		else
			r.status = BD_NO_FILE;
	}
	else if (!LocalFileName(name))
		r.status = BD_NO_FILE;
	else
	{
		const int size = ReadPublisherFile(name, data, maxStorageFile + 1);
		if (size == -1)
			r.status = BD_NO_FILE;
		else if (size < 0 || unsigned(size) > maxStorageFile)
			r.status = BD_SERVICE_NOT_AVAILABLE;
		else
			FileResult(r, data, unsigned(size));
	}
	Free(data);
}

void HandleTask(u8 service, u8 task, const u8 *p, unsigned n, Reply &r)
{
	if (service == 0x26 && task == 4)
		return;  // the console report: acknowledged, not kept
	if (service == 0x44 && task == 1)
		r.status = UpdatePresence(p, n);
	else if (service == 0x44 && task == 2)
		QueryPresence(p, n, r);
	else if (service == 0x51 && (task == 2 || task == 6 || task == 8))
		return;  // the local profile has no league teams or team members
	else if (service == 0x0A && task == 1)
		UploadProfileFile(p, n, r);
	else if (service == 0x0A && task == 3)
		GetProfileFile(p, n, r);
	else if (service == 0x0A && task == 7)
		GetPublisherFile(p, n, r);
	else if (service == 0x0C && task == 6)
	{
		r.results.Byte(8);
		r.results.U32(unsigned(UnixTime()));
		r.count = 1;
	}
	else if (service == 0x1B && task == 3)
	{
		// bdDMLHierarchicalInfo: four location strings, two float32 coordinates, five uint32 hierarchy fields
		static const char location[] = "\x10ZZ\0\x10Local\0\x10Local\0\x10Local";
		r.results.Bytes(location, sizeof(location));
		static const u8 tags[] = {13, 13, 8, 8, 8, 8, 8};
		for (u8 tag : tags)
		{
			r.results.Byte(tag);
			r.results.U32(0);
		}
		r.count = 1;
	}
	else
		r.status = BD_SERVICE_NOT_AVAILABLE;
}

void Iv8(const u8 seed[4], u8 iv[8])
{
	u8 full[24];
	InitialVector(U32(seed), full);
	memcpy(iv, full, 8);
}
}  // namespace

unsigned AnswerAuth(unsigned titleId, unsigned ivSeed, char *out, unsigned capacity)
{
	// The client ticket as the Store parser reads it (128 bytes); the server ticket is opaque to the client
	u8 client[ticketSize] = {0xDE, 0xAD, 0xBD, 0xEF}, server[ticketSize];
	Out c{client + 5, 0, 4};
	c.U32(titleId);
	Out u{client + 25, 0, 8};
	u.U64(localUserID);
	memcpy(client + 33, "Local Player", 12);
	if (!titleId || !Random(client + 97, keySize) || !Random(server, ticketSize))
		return 0;
	if (!AddTicket(server, titleId, client + 97, MonotonicMs()))
		return 0;

	Out json{reinterpret_cast<u8 *>(out), 0, capacity};
	char number[16];
	json.Text("{\"auth_task\":45,\"code\":700,\"iv_seed\":");
	unsigned i = sizeof(number);
	number[--i] = 0;
	do
		number[--i] = char('0' + ivSeed % 10);
	while (ivSeed /= 10);
	json.Text(number + i);
	json.Text(",\"client_ticket\":\"");
	Base64(json, client, ticketSize);
	json.Text("\",\"server_ticket\":\"");
	Base64(json, server, ticketSize);
	json.Text("\",\"extra_data\":\"{\\\"sbx\\\":\\\"RETAIL\\\",\\\"dpi\\\":\\\"LOCAL\\\"}\"}\n");
	json.Byte(0);
	return json.ok ? json.size - 1 : 0;
}

bool Handshake(const u8 hello[8], const u8 *frame, unsigned size, Session &session, u8 reply[15])
{
	static const u8 expected[8] = {200, 0, 0, 0, 255, 255, 0, 0};
	if (memcmp(hello, expected, 8) || size != 140 || frame[0] != 0 || frame[1] != 7)
		return false;
	const u8 *bits = frame + 2;
	auto read = [bits](unsigned start, unsigned count) {
		unsigned v = 0;
		for (unsigned i = 0; i < count; ++i)
			v |= unsigned(bits[(start + i) / 8] >> ((start + i) % 8) & 1) << i;
		return v;
	};
	if (read(0, 1) != 1 || read(1, 5) != 8 || read(38, 5) != 8)
		return false;
	const unsigned title = read(6, 32);
	u8 ticket[ticketSize];
	for (unsigned i = 0; i < ticketSize; ++i)
		ticket[i] = u8(read(75 + i * 8, 8));
	if (!ConsumeTicket(ticket, title, MonotonicMs(), session.key))
	{
		Trace("T6 Demonware: lobby rejected an unknown or expired ticket\n");
		return false;
	}
	Lock();
	session.id = ++s_nextConnection;
	Unlock();
	const u8 head[7] = {11, 0, 0, 0, 0, 4, 10};
	memcpy(reply, head, 7);
	Out id{reply + 7, 0, 8};
	id.U64(session.id);
	return true;
}

int HandleFrame(Session &session, const u8 *frame, unsigned size, u8 *reply)
{
	if (!size)
	{
		memset(reply, 0, 4);  // a keepalive
		return 4;
	}
	if (size < 5 + 8 || frame[0] != 1 || (size - 5) % 8)
		return -1;
	u8 *plain = static_cast<u8 *>(Allocate(size));
	if (!plain)
		return -1;
	memcpy(plain, frame + 5, size - 5);
	u8 iv[8], mac[20];
	Iv8(frame + 1, iv);
	const unsigned plainSize = size - 5;
	if (!TripleDesCbc(false, session.key, iv, plain, plainSize) ||
	    !HmacSha1(session.key, keySize, plain + 5, plainSize - 5, mac) || memcmp(plain, mac, 4))
	{
		Free(plain);
		Trace("T6 Demonware: lobby rejected a frame\n");
		return -1;
	}
	// The task: service, the typed task ID, its payload. The bandwidth test writes both IDs untyped, and shares
	// the typed tasks' reply queue, so it gets its reply in order too.
	const u8 *task = plain + 4;
	const unsigned taskSize = plainSize - 4;
	u8 service, id;
	const u8 *payload;
	unsigned payloadSize;
	if (taskSize >= 2 && task[0] == 0x12 && task[1] == 1)
		service = task[0], id = task[1], payload = task + 2, payloadSize = taskSize - 2;
	else if (taskSize >= 3 && task[1] == 3)
		service = task[0], id = task[2], payload = task + 3, payloadSize = taskSize - 3;
	else
	{
		Free(plain);
		return 0;
	}

	u8 *results = static_cast<u8 *>(Allocate(frameLimit));
	if (!results)
	{
		Free(plain);
		return -1;
	}
	Reply r{BD_NO_ERROR, {results, 0, frameLimit}, 0};
	HandleTask(service, id, payload, payloadSize, r);
	Free(plain);
	if (!r.results.ok)
		r = {BD_SERVICE_NOT_AVAILABLE, {results, 0, frameLimit}, 0};

	// The reply: the magic word, a typed transaction ID and status, the task, then the results' count (twice,
	// typed) and the results
	u8 seed[4];
	if (!Random(seed, 4))
	{
		Free(results);
		return -1;
	}
	Lock();
	const u64 transaction = ++s_nextTransaction;
	Unlock();
	Out out{reply + 9, 0, frameLimit - 5};
	out.U32(0xDEADBEEF);
	out.Byte(1);
	out.Byte(10);
	out.U64(transaction);
	out.Byte(8);
	out.U32(r.status);
	out.Byte(3);
	out.Byte(id);
	if (r.status == BD_NO_ERROR)
	{
		out.Byte(8);
		out.U32(r.count);
		if (r.count)
		{
			out.Byte(8);
			out.U32(r.count);
			out.Bytes(results, r.results.size);
		}
	}
	Free(results);
	while (out.ok && out.size % 8)
		out.Byte(0);
	if (!out.ok)
		return -1;
	Iv8(seed, iv);
	if (!TripleDesCbc(true, session.key, iv, reply + 9, out.size))
		return -1;
	Out head{reply, 0, 9};
	head.U32(5 + out.size);
	head.Byte(1);
	head.Bytes(seed, 4);
	char event[96] = "T6 Demonware: lobby task ";
	static const char hex[] = "0123456789abcdef";
	char *e = event + strlen(event);
	*e++ = hex[service >> 4], *e++ = hex[service & 15], *e++ = '/';
	*e++ = hex[id >> 4], *e++ = hex[id & 15];
	strcpy(e, r.status ? " failed\n" : " answered\n");
	Trace(event);
	return int(9 + out.size);
}
}  // namespace LocalService
#endif
