// live/live_anticheat.cpp -- Store MP CRC buffer reconstruction.
// Reference: CoDMPServer_PC, lines 535 and 553; Store MP continuation assembly.
// The offline lifecycle is deliberately not an ABI-compatible game/service hook.
#define T6_SOURCE "live/live_anticheat.cpp"
#include "live/live_anticheat.h"
#include <stdint.h>
#include <limits>
#include <algorithm>

namespace {
struct CrcTable {
    unsigned int entries[256];
    constexpr CrcTable() : entries{} {
        for (unsigned int i = 0; i < 256; ++i) {
            unsigned int crc = i;
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
            entries[i] = crc;
        }
    }
};
constexpr CrcTable table{};
static_assert(sizeof(unsigned int) == 4, "CRC32 state width");
static_assert(sizeof(unsigned long long) == 8, "split CRC state width");
unsigned int Step(unsigned int crc, unsigned char byte) {
    return (crc >> 8) ^ table.entries[(crc & 0xffu) ^ byte];
}
}

void LiveAntiCheat_GenerateCRC32Checksum(const unsigned char *data, int length, unsigned int *key) {
    unsigned int crc = *key;
    // Original caller contract: length is nonnegative.
    while (length != 0) {
        crc = Step(crc, *data++);
        --length;
    }
    *key = crc;
}

unsigned long long LiveAntiCheat_GenerateCRC32ChecksumSplit(const unsigned char *data, int length, unsigned long long key) {
    unsigned int low = static_cast<unsigned int>(key);
    unsigned int high = static_cast<unsigned int>(key >> 32);
    while (length != 0) {
        if ((reinterpret_cast<uintptr_t>(data) & 1u) == 0)
            low = Step(low, *data);
        else
            high = Step(high, *data);
        ++data;
        --length;
    }
    return (static_cast<unsigned long long>(high) << 32) | low;
}

namespace t6::checksum {
bool BufferHashPool::Owns(const StoreHashRecord *record) const {
    for (const auto &entry : records_)
        if (&entry == record) return true;
    return false;
}
StoreHashRecord *BufferHashPool::Start(Kind kind, std::uint32_t offset, std::uint32_t length, std::uint64_t seed) {
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
bool BufferHashPool::Continue(StoreHashRecord *record, const unsigned char *buffer, std::size_t bufferSize, bool &finished) {
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
bool BufferHashPool::Complete(StoreHashRecord *record, std::uint64_t &result) {
    // The standalone buffer API rejects premature completion instead of exposing a partial digest.
    if (!Owns(record) || (record->kind != Kind::Crc32 && record->kind != Kind::SplitCrc32) || record->position != record->length) return false;
    result = record->state;
    *record = {};
    return true;
}
}
