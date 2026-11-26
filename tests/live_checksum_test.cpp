// Standalone arithmetic regression; not an original-binary differential test.
#include "live/live_anticheat.h"
#include <cassert>
#include <cstdint>
#include <initializer_list>

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
    const unsigned char sample[] = "123456789";
    LiveAntiCheat_GenerateCRC32Checksum(sample, 9, &crc);
    assert((crc ^ 0xffffffffu) == 0xcbf43926u);
}
