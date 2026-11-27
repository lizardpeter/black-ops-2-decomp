// Replay probe: live/live_anticheat.cpp's LiveAntiCheat_StartHash and LiveAntiCheat_ContinueHash (both CRC loops
// inlined there) against the Store MP original. verify_live_anticheat.py drives it.
#include "live/live_anticheat.h"

#include <cstdint>
#include <cstring>
#include <sys/mman.h>

#define MS __attribute__((ms_abi))

HashInfo s_hashInfos[16];  // the reconstruction's; the original's is at 14426E340

namespace
{
constexpr unsigned STORE_HASH_INFOS = 0x426E340;
unsigned char *buffer;  // below 4 GB: the context keeps a 32-bit address
unsigned state;
unsigned Next() { return state = state * 1664525u + 1013904223u; }

struct Result
{
	long long handle;  // the returned context's index, -1 for NULL
	unsigned char ret;
	HashInfo infos[16];
};
}  // namespace

extern "C" int AntiCheatInit()
{
	void *p = mmap(nullptr, 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
	if (p == MAP_FAILED)
		return 0;
	buffer = static_cast<unsigned char *>(p);
	return 1;
}

// op 0 LiveAntiCheat_StartHash, 1 LiveAntiCheat_ContinueHash; image null: the reconstruction.
extern "C" void AntiCheatRun(unsigned char *image, unsigned op, unsigned seed, Result *out)
{
	state = seed + 1;
	HashInfo infos[16] = {};
	const unsigned used = Next() % 17;
	for (unsigned i = 0; i < 16; ++i)
		if (i < used || Next() % 4 == 0)
			infos[i] = {CT_TYPE_CRC32, Next(), Next() % 64, 0, 0x2000, ((unsigned long long)Next() << 32) | Next()};
	for (unsigned i = 0; i < 0x8000; ++i)
		buffer[i] = (unsigned char)Next();
	const unsigned slot = Next() % 16;
	const challengeTest_t type = (challengeTest_t)(Next() % 6);
	const unsigned length = Next() % 0x5000, position = length ? Next() % (length + 1) : 0;
	const unsigned block = (Next() % 3) ? 0x2000 : 1 + Next() % 64;
	const unsigned long long key = ((unsigned long long)Next() << 32) | Next();
	if (op == 1)
		infos[slot] = {type == CT_TYPE_INVALID ? CT_TYPE_CRC32 : (type > 3 ? CT_TYPE_CRC32_SPLIT : type),
		               (unsigned)(std::uintptr_t)buffer + (Next() & 7), length, position, block, key};
	const unsigned start = (unsigned)(std::uintptr_t)buffer + (Next() & 7);
	HashInfo *table = image ? reinterpret_cast<HashInfo *>(image + STORE_HASH_INFOS) : s_hashInfos;
	std::memcpy(table, infos, sizeof(infos));
	Result r{};
	if (op == 0)
	{
		void *h = image ? reinterpret_cast<void *(MS *)(challengeTest_t, unsigned, unsigned, unsigned, unsigned long long)>(
		                      image + 0x576490)(type, start, length, 0x2000, key)
		                : LiveAntiCheat_StartHash(type, start, length, 0x2000, key);
		r.handle = h ? static_cast<HashInfo *>(h) - table : -1;
	}
	else
	{
		r.handle = slot;
		r.ret = image ? reinterpret_cast<bool(MS *)(void *)>(image + 0x576540)(&table[slot])
		              : LiveAntiCheat_ContinueHash(&table[slot]);
	}
	std::memcpy(r.infos, table, sizeof(r.infos));
	*out = r;
}
