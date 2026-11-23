// Original Store binkw64.dll FUN_180024FF0, RVA 0x24FF0, transcribed
// from the full Ghidra decompilation of SHA-256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// An internally buffered 16-bit coefficient delta/RLE decoder:
//  * Reads a block count, bounded by decoder capacity / sizeof(int16_t).
//  * Reads a starting coefficient, either unsigned or sign-magnitude.
//  * For each group of <=8 remaining coefficients, reads a four-bit bit
//    width. Width zero repeats the last coefficient. Otherwise each value
//    uses that many magnitude bits, followed by one sign bit if nonzero.
//  * Adds deltas modulo 2^32 and stores the low 16 bits of each result.
//
// Implements original buffer-layout and LSB-first packed-bit semantics.
// Real game streams, invalid bit widths and truncated input are NOT fully
// validated; do not expose this as an untrusted-file parser or DLL replacement.
// No external codec or replacement decoder source has been used.
#include "entropy_bit_cursor.h"
#include <cstdint>
#include <cstring>

namespace {
static void store16(std::uint8_t* dst, std::uint32_t word) {
    const std::uint16_t value = static_cast<std::uint16_t>(word);
    std::memcpy(dst, &value, sizeof(value));
}
}

extern "C" void bink_decode_entropy_deltas16(
        t6_bink::ByteEntropyState* context,
        t6_bink::WordBitStream* stream,
        const std::uint32_t* word_end) {
    if (context->current != context->end) return;

    t6_bink::internal::BitCursor bits(*stream);
    const std::uint32_t count = bits.read(context->count_bits);
    if (count == 0 || count > (context->capacity >> 1)) {
        context->current = context->buffer + 4;
        context->end = context->buffer;
        bits.save();
        return;
    }

    // The original uses the low 32 bits of the field at +0x10 as the
    // starting-value width and the high 32 bits (+0x14) as its sign mode.
    const std::uint32_t start_width =
        static_cast<std::uint32_t>(context->reserved10);
    const std::uint32_t mode =
        static_cast<std::uint32_t>(context->reserved10 >> 32);

    std::uint32_t last = 0;
    if (mode == 0) {
        last = bits.read(start_width);
    } else {
        last = bits.read(start_width - 1);
        if (last != 0 && bits.read(1) != 0)
            last = std::uint32_t{0} - last;
    }

    context->current = context->buffer;
    context->end = context->buffer + 2U * count;
    store16(context->buffer, last);

    std::uint32_t index = 1;
    while (index < count) {
        // In the original, the length of a group is min(8, count-index)
        // and the four-bit group width is read once per group.
        const std::uint32_t length =
            count - index < 8U ? count - index : 8U;
        if (bits.available() < 4U && bits.next() >= word_end) {
            context->current = context->buffer + 4;
            context->end = context->buffer;
            bits.save();
            return;
        }
        const std::uint32_t delta_width = bits.read(4);
        if (delta_width == 0) {
            for (std::uint32_t j = 0; j < length; ++j)
                store16(context->buffer + 2U * (index + j), last);
        } else {
            for (std::uint32_t j = 0; j < length; ++j) {
                std::uint32_t magnitude = bits.read(delta_width);
                if (magnitude != 0 && bits.read(1) != 0)
                    magnitude = std::uint32_t{0} - magnitude;
                last += magnitude;
                store16(context->buffer + 2U * (index + j), last);
            }
        }
        index += length;
    }
    bits.save();
}
