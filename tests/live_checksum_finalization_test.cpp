// Native helper regression. No game process, network or original code fixture.
#include "live/live_anticheat.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>

int main()
{
    unsigned crcCases = 0, nonCrcCases = 0;
    for (auto type : {CT_TYPE_CRC32, CT_TYPE_CRC32_SPLIT, CT_TYPE_SHA})
    for (unsigned length : {0u, 1u, 8192u, 8193u, 65535u})
    for (unsigned position : {0u, 1u, 8192u, 8193u, 65535u})
    for (auto seed : {0ull, 1ull, 0xffffffffull, 0xffffffffffffffffull, 0xdeadbeef12345678ull})
    for (auto initial : {0ull, 0x1020304050607080ull, 0xffffffffffffffffull}) {
        if (position > length) continue;
        auto *record = static_cast<HashInfo *>(LiveAntiCheat_StartHash(type, 0, length, 8192, seed));
        assert(record);
        record->position = position;
        record->state = seed;
        record->reserved = 0xa5a5a5a5;
        unsigned long long result = initial;
        assert(LiveAntiCheat_CompleteHash(record, &result));
        const HashInfo empty{};
        assert(!std::memcmp(record, &empty, sizeof(empty)));
        if (type == CT_TYPE_SHA) {
            // Standalone reference helper does not publish a digest for this mode.
            // Not a claim about an inlined caller's output.
            assert(result == initial);
            ++nonCrcCases;
        } else {
            assert(result == seed);
            ++crcCases;
        }
        auto *reused = static_cast<HashInfo *>(LiveAntiCheat_StartHash(CT_TYPE_CRC32, 0, 0, 8192, 0));
        assert(reused == record);
        assert(LiveAntiCheat_CompleteHash(reused, &result) && result == 0);
    }
    assert(crcCases == 450 && nonCrcCases == 225);
    std::printf("%u CRC finalization cases; %u standalone non-CRC cases; slot reuse passed\n", crcCases, nonCrcCases);
}
