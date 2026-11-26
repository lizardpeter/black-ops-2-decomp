// Offline checksum arithmetic and bounded lifecycle regression.
#include "live_checksum_model.h"
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <cstdio>
#include <vector>
#include <array>
#include <cstring>

static unsigned int bitwise(unsigned int crc, unsigned char value) {
    crc ^= value;
    for (int i = 0; i < 8; ++i)
        crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    return crc;
}
int main() {
    alignas(2) unsigned char bytes[258]{};
    for (int i = 0; i < 258; ++i) bytes[i] = static_cast<unsigned char>(i * 73 + 19);
    unsigned int cases = 0;
    for (int offset = 0; offset < 2; ++offset) {
        for (int n = 0; n <= 256; ++n) {
            for (unsigned int seed : {0u, 0xffffffffu, 0x12345678u}) {
                auto data = bytes + offset;
                unsigned int expected = seed, lo = seed, hi = seed ^ 0xabcdef01u;
                for (int i = 0; i < n; ++i) {
                    expected = bitwise(expected, data[i]);
                    if ((reinterpret_cast<std::uintptr_t>(data+i) & 1u) == 0) lo = bitwise(lo, data[i]);
                    else hi = bitwise(hi, data[i]);
                }
                unsigned int actual = seed;
                LiveAntiCheat_GenerateCRC32Checksum(data, n, &actual);
                assert(actual == expected);
                auto key = (static_cast<unsigned long long>(seed ^ 0xabcdef01u) << 32) | seed;
                auto split = LiveAntiCheat_GenerateCRC32ChecksumSplit(data, n, key);
                assert(split == ((static_cast<unsigned long long>(hi) << 32) | lo));
                auto mid = LiveAntiCheat_GenerateCRC32ChecksumSplit(data, n/2, key);
                assert(split == LiveAntiCheat_GenerateCRC32ChecksumSplit(data+n/2, n-n/2, mid));
                ++cases;
            }
        }
    }
    assert(cases == 1542);
    unsigned int crc = 0xffffffffu;
    unsigned char sample[] = "123456789";
    LiveAntiCheat_GenerateCRC32Checksum(sample, 9, &crc);
    assert((crc ^ 0xffffffffu) == 0xcbf43926u);
    using namespace t6::checksum;
    std::vector<unsigned char> large(0x6004);
    for (std::size_t i = 0; i < large.size(); ++i) large[i] = static_cast<unsigned char>(i*97+31);
    unsigned lifecycle = 0;
    for (auto kind : {Kind::Crc32, Kind::SplitCrc32}) for (unsigned offset : {0u, 1u, 2u, 3u})
    for (unsigned length : {0u, 1u, 2u, 0x1fffu, 0x2000u, 0x2001u, 0x4000u, 0x4001u, 0x6000u})
    for (auto seed : {0ull, 0xffffffffffffffffull, 0x123456789abcdef0ull}) {
        BufferHashPool pool;
        auto *r = pool.Start(kind, offset, length, seed);
        assert(r && r->position == 0 && r->state == seed);
        bool finished = false;
        std::uint64_t result = 0;
        if (length) assert(!pool.Complete(r, result));
        unsigned steps = 0;
        unsigned lo = static_cast<unsigned>(seed), hi = static_cast<unsigned>(seed >> 32);
        do {
            auto begin = r->position;
            assert(pool.Continue(r, large.data(), large.size(), finished));
            for (unsigned i = begin; i < r->position; ++i) {
                auto p = large.data() + offset + i;
                if (kind == Kind::Crc32 || !(reinterpret_cast<std::uintptr_t>(p) & 1)) lo = bitwise(lo, *p);
                else hi = bitwise(hi, *p);
            }
            if (kind == Kind::Crc32 && length) hi = 0;
            assert(r->state == ((static_cast<std::uint64_t>(hi) << 32) | lo));
            ++steps;
        } while (!finished);
        assert(steps == (length ? (length + 0x1fff) / 0x2000 : 1));
        assert(pool.Complete(r, result));
        assert(result == ((static_cast<std::uint64_t>(hi) << 32) | lo));
        assert(r->kind == Kind::Free && r->state == 0 && r->length == 0);
        assert(!pool.Complete(r, result));
        ++lifecycle;
    }
    BufferHashPool pool;
    std::array<StoreHashRecord*,16> slots{};
    for (auto &slot : slots) { slot = pool.Start(Kind::Crc32, 0, 0, 0); assert(slot); }
    assert(!pool.Start(Kind::Crc32, 0, 0, 0));
    std::uint64_t result = 0;
    assert(pool.Complete(slots[5], result));
    assert(pool.Start(Kind::SplitCrc32, 1, 2, 0) == slots[5]);
    bool finished;
    assert(!pool.Continue(slots[5], large.data(), 2, finished));
    assert(!pool.Continue(nullptr, large.data(), large.size(), finished));
    assert(!pool.Start(static_cast<Kind>(2), 0, 0, 0));
    std::printf("%u arithmetic cases; %u lifecycle cases; pool/bounds checks passed\n", cases, lifecycle);
}
