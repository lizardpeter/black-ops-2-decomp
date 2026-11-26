// Offline buffer model regression; never attaches to a game or service.
#include "live_checksum_model.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>

using namespace t6::checksum;
static std::uint32_t step(std::uint32_t crc, unsigned char byte) {
    crc ^= byte;
    for (unsigned i = 0; i != 8; ++i)
        crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    return crc;
}
static std::uint64_t expected(Kind kind, const unsigned char *p,
                              unsigned n, std::uint64_t seed) {
    auto low = static_cast<std::uint32_t>(seed);
    auto high = static_cast<std::uint32_t>(seed >> 32);
    for (unsigned i = 0; i != n; ++i) {
        if (kind == Kind::Crc32 || (reinterpret_cast<std::uintptr_t>(p+i) & 1u) == 0)
            low = step(low, p[i]);
        else high = step(high, p[i]);
    }
    if (kind == Kind::Crc32 && n != 0) high = 0;
    return (static_cast<std::uint64_t>(high) << 32) | low;
}
static bool same(const StoreHashRecord &a, const StoreHashRecord &b) {
    return a.kind == b.kind && a.start == b.start && a.length == b.length &&
        a.position == b.position && a.blockSize == b.blockSize &&
        a.reserved == b.reserved && a.state == b.state;
}
int main() {
    std::vector<unsigned char> bytes(65536);
    std::uint32_t rng = 0x1badb002u;
    auto next = [&] { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng; };
    for (auto &b : bytes) b = static_cast<unsigned char>(next());
    const auto saved = bytes;
    unsigned completed = 0, calls = 0;
    for (unsigned round = 0; round != 32; ++round) {
        BufferHashPool pool;
        std::array<StoreHashRecord *, 16> slots{};
        std::array<std::uint64_t, 16> answers{};
        for (unsigned i = 0; i != slots.size(); ++i) {
            auto kind = i & 1 ? Kind::Crc32 : Kind::SplitCrc32;
            unsigned offset = next() % 4096;
            unsigned length = next() % 61441;
            auto seed = (static_cast<std::uint64_t>(next()) << 32) | next();
            slots[i] = pool.Start(kind, offset, length, seed);
            assert(slots[i]);
            answers[i] = expected(kind, bytes.data() + offset, length, seed);
        }
        assert(!pool.Start(Kind::Crc32, 0, 0, 0));
        unsigned active = slots.size();
        while (active != 0) {
            for (unsigned i = 0; i != slots.size(); ++i) {
                auto *r = slots[i];
                if (!r) continue;
                bool done = false;
                assert(pool.Continue(r, bytes.data(), bytes.size(), done));
                ++calls;
                if (!done) continue;
                std::uint64_t out = 0;
                assert(pool.Complete(r, out));
                assert(out == answers[i]);
                slots[i] = nullptr;
                --active; ++completed;
            }
        }
    }
    assert(bytes == saved);
    BufferHashPool pool, other;
    auto *r = pool.Start(Kind::Crc32, 7, 12, 42);
    assert(r);
    auto before = *r;
    bool done = true;
    std::uint64_t result = 0x1122334455667788ull;
    assert(!other.Continue(r, bytes.data(), bytes.size(), done) && !done);
    assert(!other.Complete(r, result));
    assert(!pool.Complete(r, result));
    assert(result == 0x1122334455667788ull && same(*r, before));
    assert(!pool.Continue(r, bytes.data(), 18, done) && !done);
    assert(!pool.Continue(r, nullptr, bytes.size(), done) && !done);
    assert(same(*r, before));
    r->position = 13;
    auto invalid = *r;
    assert(!pool.Continue(r, bytes.data(), bytes.size(), done) && !done && same(*r, invalid));
    *r = before;
    r->blockSize = 0;
    invalid = *r;
    assert(!pool.Continue(r, bytes.data(), bytes.size(), done) && !done && same(*r, invalid));
    *r = before;
    assert(pool.Continue(r, bytes.data(), bytes.size(), done) && done);
    assert(pool.Complete(r, result));
    assert(!pool.Start(Kind::Crc32, 0, std::numeric_limits<std::uint32_t>::max(), 0));
    r = pool.Start(Kind::SplitCrc32, 0, 0, 7);
    assert(r && pool.Continue(r, nullptr, 0, done) && done);
    assert(pool.Complete(r, result) && result == 7);
    std::printf("%u interleaved lifecycles; %u continuation calls; rejection/input-preservation checks passed\n", completed, calls);
}
