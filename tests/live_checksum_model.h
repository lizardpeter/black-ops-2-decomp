#pragma once
#include "live/live_anticheat.h"
#include <algorithm>
#include <limits>

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

namespace t6::checksum {
inline bool BufferHashPool::Owns(const StoreHashRecord *record) const {
    for (const auto &entry : records_)
        if (&entry == record) return true;
    return false;
}
inline StoreHashRecord *BufferHashPool::Start(Kind kind, std::uint32_t offset, std::uint32_t length, std::uint64_t seed) {
    // The unsupported non-CRC mode is not exposed by the offline interface.
    if (kind != Kind::Crc32 && kind != Kind::SplitCrc32) return nullptr;
    if (length > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) return nullptr;
    for (auto &entry : records_) {
        if (entry.kind != Kind::Free) continue;
        entry = {kind, offset, length, 0, 0x2000, 0, seed};
        return &entry;
    }
    return nullptr;
}
inline bool BufferHashPool::Continue(StoreHashRecord *record, const unsigned char *buffer, std::size_t bufferSize, bool &finished) {
    finished = false;
    if (!Owns(record) || (record->kind != Kind::Crc32 && record->kind != Kind::SplitCrc32)) return false;
    if (record->position > record->length || record->blockSize != 0x2000) return false;
    if (record->start > bufferSize || record->length > bufferSize - record->start) return false;
    if (record->position == record->length) { finished = true; return true; }
    if (!buffer) return false;
    auto count = std::min(record->blockSize, record->length - record->position);
    auto data = buffer + static_cast<std::size_t>(record->start) + record->position;
    if (record->kind == Kind::Crc32) {
        auto state = static_cast<unsigned int>(record->state);
        LiveAntiCheat_GenerateCRC32Checksum(data, static_cast<int>(count), &state);
        record->state = state;
    } else {
        record->state = LiveAntiCheat_GenerateCRC32ChecksumSplit(data, static_cast<int>(count), record->state);
    }
    record->position += count;
    finished = record->position == record->length;
    return true;
}
inline bool BufferHashPool::Complete(StoreHashRecord *record, std::uint64_t &result) {
    // The standalone buffer API rejects premature completion instead of exposing a partial digest.
    if (!Owns(record) || (record->kind != Kind::Crc32 && record->kind != Kind::SplitCrc32) || record->position != record->length) return false;
    result = record->state;
    *record = {};
    return true;
}
}
