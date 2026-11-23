// Original Bink video bit-mask byte interleaver, FUN_1800237B0 (RVA 0x237B0).
// Original Store DLL SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Reconstructed directly from its machine-code-derived Ghidra control flow.
// Used in the video decoder's byte-processing path. Not a third-party decoder.
// Interleaves two count-byte streams according to the low bits of mask. Once
// either source has provided count bytes, appends the remainder of the other.
// Return value is the number of mask-controlled output bytes (NOT 2*count).
//
// Fidelity to the original: mask is truncated to 31 low bits after EVERY
// consumed bit (even if its argument was 64-bit). The original assumes count
// is positive, and buffers have valid sizes; no synthetic null/length guards.
#include <cstdint>

extern "C" int bink_video_interleave_bytes(std::uint64_t mask,
                                            std::uint8_t* dst,
                                            const std::uint8_t* first,
                                            const std::uint8_t* second,
                                            int count) {
    int remaining_first = count;
    int remaining_second = count;
    int selections = 0;
    for (;;) {
        ++selections;
        if ((mask & 1U) == 0) {
            *dst++ = *first++;
            --remaining_first;
        } else {
            *dst++ = *second++;
            --remaining_second;
        }
        mask = (mask >> 1) & UINT64_C(0x7fffffff);
        if (remaining_first == 0) break;
        if (remaining_second == 0) {
            do {
                *dst++ = *first++;
                --remaining_first;
            } while (remaining_first != 0);
            return selections;
        }
    }
    while (remaining_second != 0) {
        *dst++ = *second++;
        --remaining_second;
    }
    return selections;
}
