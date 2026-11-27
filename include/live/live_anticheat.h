#pragma once
#include <cstdint>
#include <cstddef>

void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key);
unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length, unsigned long long key);

// The Store compiler preserves a 32-bit address field in this x64 record.
enum challengeTest_t : int {
    CT_TYPE_INVALID = 0,
    CT_TYPE_CRC32 = 1,
    CT_TYPE_SHA = 2,
    CT_TYPE_CRC32_SPLIT = 3,
};
struct alignas(8) HashInfo {
    challengeTest_t type;
    unsigned int start;
    unsigned int length;
    unsigned int position;
    unsigned int blockSize;
    unsigned int reserved;
    unsigned long long state;
};
static_assert(sizeof(HashInfo) == 0x20);
static_assert(offsetof(HashInfo, type) == 0x00);
static_assert(offsetof(HashInfo, start) == 0x04);
static_assert(offsetof(HashInfo, length) == 0x08);
static_assert(offsetof(HashInfo, position) == 0x0c);
static_assert(offsetof(HashInfo, blockSize) == 0x10);
static_assert(offsetof(HashInfo, reserved) == 0x14);
static_assert(offsetof(HashInfo, state) == 0x18);

#ifdef T6_STORE
void *LiveAntiCheat_StartHash(challengeTest_t type, unsigned int start, unsigned int length,
                             unsigned int blockSize, unsigned long long key);
bool LiveAntiCheat_ContinueHash(void *handle);
bool LiveAntiCheat_CompleteHash(void *handle, unsigned long long *result);
#endif
