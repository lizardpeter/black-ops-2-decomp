// Store x64 original-style lifecycle, tested with private low-address buffers.
// Optional original-instruction replay fixtures are local, never committed.
#include "live/live_anticheat.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <sys/mman.h>

namespace {
class Mapping {
public:
    explicit Mapping(std::size_t n, int flags = 0) : size(n) {
        data = static_cast<unsigned char *>(mmap(nullptr, size, PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | flags, -1, 0));
        assert(data != MAP_FAILED);
    }
    ~Mapping() { munmap(data, size); }
    Mapping(const Mapping &) = delete;
    Mapping &operator=(const Mapping &) = delete;
    unsigned char *data;
    std::size_t size;
};
void read_fixture(Mapping &mapping, const char *path, std::size_t expected) {
    std::ifstream in(path, std::ios::binary);
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
    assert(bytes.size() == expected && bytes.size() <= mapping.size);
    std::memcpy(mapping.data, bytes.data(), bytes.size());
    assert(mprotect(mapping.data, 4096, PROT_READ | PROT_EXEC) == 0);
}
unsigned int step(unsigned int crc, unsigned char byte) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
    return crc;
}
}
int main(int argc, char **argv) {
    assert(argc == 1 || argc == 3);
    using Start = HashInfo *(__attribute__((ms_abi)) *)(challengeTest_t, unsigned int,
            unsigned int, unsigned int, unsigned long long);
    using Continue = bool (__attribute__((ms_abi)) *)(HashInfo *);
    Mapping startup(8192), continuation(8192), input(0x7000, MAP_32BIT);
    Start originalStart = nullptr;
    Continue originalContinue = nullptr;
    if (argc == 3) {
        read_fixture(startup, argv[1], 171);
        read_fixture(continuation, argv[2], 5120);
        assert(mprotect(continuation.data + 4096, 4096, PROT_READ) == 0);
        originalStart = reinterpret_cast<Start>(startup.data);
        originalContinue = reinterpret_cast<Continue>(continuation.data);
    }
    auto *originalRecords = reinterpret_cast<HashInfo *>(startup.data + 4096);
    const auto base = reinterpret_cast<std::uintptr_t>(input.data);
    assert(base + input.size <= 0xffffffffull);
    for (std::size_t i = 0; i < input.size; ++i)
        input.data[i] = static_cast<unsigned char>(i * 97 + 31);
    assert(mprotect(input.data, input.size, PROT_READ) == 0);

    unsigned cases = 0, calls = 0, starts = 0;
    for (unsigned round = 0; round < 64; ++round) {
        std::array<HashInfo *, 16> slots{};
        for (unsigned i = 0; i < slots.size(); ++i) {
            const auto type = static_cast<challengeTest_t>(1 + (i + round) % 3);
            const auto address = static_cast<unsigned int>(base + i);
            const auto length = (round * 997 + i * 257) % 0x6001;
            const auto seed = (0x123456789abcdef0ull * (round + 1)) ^ i;
            slots[i] = static_cast<HashInfo *>(LiveAntiCheat_StartHash(type, address, length, round, seed));
            assert(slots[i] && slots[i]->blockSize == 0x2000 && slots[i]->position == 0);
            if (originalStart) {
                auto *reference = originalStart(type, address, length, round, seed);
                assert(reference == originalRecords + i && !std::memcmp(reference, slots[i], sizeof(HashInfo)));
            }
            ++starts;
        }
        assert(!LiveAntiCheat_StartHash(CT_TYPE_CRC32, 0, 0, 0, 0));
        if (originalStart) assert(!originalStart(CT_TYPE_CRC32, 0, 0, 0, 0));
        ++starts;
        for (unsigned i = 0; i < slots.size(); ++i) {
            unsigned long long digest = 0x1122334455667788ull;
            const auto before = slots[i]->state;
            const auto type = slots[i]->type;
            // The original completion clears a record even before all data is consumed.
            assert(LiveAntiCheat_CompleteHash(slots[i], &digest));
            assert(digest == (type == CT_TYPE_SHA ? 0x1122334455667788ull : before));
            const HashInfo empty{};
            assert(!std::memcmp(slots[i], &empty, sizeof(empty)));
            if (originalStart) originalRecords[i] = {};
        }
    }
    for (auto type : {CT_TYPE_CRC32, CT_TYPE_SHA, CT_TYPE_CRC32_SPLIT})
    for (unsigned offset : {0u, 1u, 2u, 3u, 31u, 32u, 63u, 64u})
    for (unsigned length : {0u, 1u, 2u, 3u, 255u, 256u, 257u, 0x1fffu, 0x2000u, 0x2001u, 0x3fffu, 0x4000u, 0x4001u, 0x6000u})
    for (auto seed : {0ull, 0xffffffffffffffffull, 0x123456789abcdef0ull, 0x8000000000000000ull, 0x00000000ffffffffull}) {
        auto *record = static_cast<HashInfo *>(LiveAntiCheat_StartHash(type,
            static_cast<unsigned int>(base + offset), length, 123, seed));
        assert(record);
        HashInfo reference = *record;
        unsigned int low = static_cast<unsigned int>(seed), high = static_cast<unsigned int>(seed >> 32);
        if (type == CT_TYPE_SHA) low = high = 0;
        bool done = false;
        do {
            const auto begin = record->position;
            done = LiveAntiCheat_ContinueHash(record);
            if (originalContinue) {
                assert(originalContinue(&reference) == done);
                assert(!std::memcmp(&reference, record, sizeof(reference)));
            }
            for (unsigned i = begin; i < record->position && type != CT_TYPE_SHA; ++i) {
                const auto *p = input.data + offset + i;
                if (type == CT_TYPE_CRC32 || !(reinterpret_cast<std::uintptr_t>(p) & 1)) low = step(low, *p);
                else high = step(high, *p);
            }
            if (type == CT_TYPE_CRC32 && length) high = 0;
            assert(record->state == ((static_cast<unsigned long long>(high) << 32) | low));
            ++calls;
        } while (!done);
        unsigned long long digest = 0x1122334455667788ull;
        const auto expected = type == CT_TYPE_SHA ? digest : record->state;
        assert(LiveAntiCheat_CompleteHash(record, &digest) && digest == expected);
        const HashInfo empty{};
        assert(!std::memcmp(record, &empty, sizeof(empty)));
        ++cases;
    }
    // The original accepts type 2 but has no SHA computation in these bodies.
    assert(!LiveAntiCheat_StartHash(static_cast<challengeTest_t>(4), 0, 0, 0, 0));
    auto *again = LiveAntiCheat_StartHash(CT_TYPE_CRC32, 0, 0, 0, 0);
    unsigned long long out = 123;
    assert(again && LiveAntiCheat_CompleteHash(again, &out) && out == 0);
    std::printf("%u startup comparisons; %u lifecycle cases; %u continuation calls; %s\n",
        starts, cases, calls, originalStart ? "original replay and native checks passed" : "native checks passed");
}
