// live/live_anticheat.cpp: Demonware's checksum challenges.
#pragma once

enum challengeTest_t : int
{
	CT_TYPE_INVALID = 0,
	CT_TYPE_CRC32 = 1,
	CT_TYPE_SHA = 2,
	CT_TYPE_CRC32_SPLIT = 3,
};

// One checksum in progress (0x20 bytes in every build: the address range is 32-bit even in the Store build).
struct HashInfo
{
	challengeTest_t type;
	unsigned int start;
	unsigned int length;
	unsigned int position;
	unsigned int blockSize;
	unsigned long long state;
};
static_assert(sizeof(HashInfo) == 0x20 && __builtin_offsetof(HashInfo, state) == 0x18, "HashInfo");

void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key);
unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length,
                                                            unsigned long long key);
void *LiveAntiCheat_StartHash(challengeTest_t type, unsigned int start, unsigned int length, unsigned int blockSize,
                              unsigned long long key);
bool LiveAntiCheat_ContinueHash(void *handle);
bool LiveAntiCheat_CompleteHash(void *handle, unsigned long long *result);
