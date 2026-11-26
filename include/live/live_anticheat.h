#pragma once
#include <cstdint>
#include <cstddef>

void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key);
unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length, unsigned long long key);

// Offline, caller-owned buffer model of the Store MP CRC hash lifecycle.
// Not an ABI-compatible game/service entry point. The start field is an offset
// into the supplied buffer, not an absolute address in another process.
namespace t6::checksum {
enum class Kind : std::uint32_t { Free = 0, Crc32 = 1, SplitCrc32 = 3 };
struct alignas(8) StoreHashRecord {
    Kind kind;
    std::uint32_t start;
    std::uint32_t length;
    std::uint32_t position;
    std::uint32_t blockSize;
    std::uint32_t reserved;
    std::uint64_t state;
};
static_assert(sizeof(StoreHashRecord) == 0x20);
static_assert(offsetof(StoreHashRecord, kind) == 0);
static_assert(offsetof(StoreHashRecord, start) == 4);
static_assert(offsetof(StoreHashRecord, length) == 8);
static_assert(offsetof(StoreHashRecord, position) == 12);
static_assert(offsetof(StoreHashRecord, blockSize) == 16);
static_assert(offsetof(StoreHashRecord, reserved) == 20);
static_assert(offsetof(StoreHashRecord, state) == 24);

class BufferHashPool {
public:
    StoreHashRecord *Start(Kind kind, std::uint32_t offset, std::uint32_t length, std::uint64_t seed);
    bool Continue(StoreHashRecord *record, const unsigned char *buffer, std::size_t bufferSize, bool &finished);
    bool Complete(StoreHashRecord *record, std::uint64_t &result);
private:
    bool Owns(const StoreHashRecord *record) const;
    StoreHashRecord records_[16]{};
};
}
