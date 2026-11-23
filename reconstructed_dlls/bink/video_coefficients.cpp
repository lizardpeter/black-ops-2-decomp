// Original Bink coefficient bit-plane decoder, FUN_18002FD80 (RVA 0x2FD80).
// Translated from pinned original Binkw64.dll x64 instructions and Ghidra
// machine-code decompilation. No vendor SDK, FFmpeg or replacement codec.
// Original SHA256: 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
//
// Input: 128-byte coefficient buffer; the first 16-bit coefficient is already
// populated by the caller and MUST be preserved. BitStream is original
// 24-byte LSB-first word reservoir. Remaining 63 int16 coefficients are
// decoded through hierarchical 2-bit token kinds (group, expand, 4-block,
// single coefficient), then permuted into original output order.
// This faithfully preserves the original buffer and bitstream ABI for valid
// input; callers must provide padded data and a correctly initialized cache.
#include "entropy_bit_cursor.h"
#include <array>
#include <cstdint>
#include <cstring>

namespace {
// Original FUN_18002FD80's 32 dword output copies permute the temporary
// 64-coefficient array. Each element is the INTERNAL source coefficient for
// corresponding output int16 index. Output index zero is never written.
constexpr std::array<std::uint8_t,64> original_output_permutation = {
   0,  1,  4,  5,  8,  9, 12, 13,  2,  3,  6,  7, 10, 11, 14, 15,
  24, 25, 44, 45, 16, 17, 20, 21, 26, 27, 46, 47, 18, 19, 22, 23,
  28, 29, 32, 33, 48, 49, 52, 53, 30, 31, 34, 35, 50, 51, 54, 55,
  36, 37, 40, 41, 56, 57, 60, 61, 38, 39, 42, 43, 58, 59, 62, 63,
};
} // namespace

extern "C" void bink_video_decode_coefficients(
        std::uint8_t* output,
        t6_bink::WordBitStream* bitstream) {
    t6_bink::internal::BitCursor bits(*bitstream);
    const std::uint32_t top_bitplane = bits.read(4);
    std::array<std::uint16_t,64> coefficients{};
    // Original hierarchy starts with three 4-coefficient groups followed
    // by three single coefficients. The 2-bit token kind is in its low bits.
    std::array<std::uint8_t,256> tokens{};
    std::uint32_t begin = 128;
    std::uint32_t end = begin + 6;
    tokens[begin+0]=0x10;
    tokens[begin+1]=0x60;
    tokens[begin+2]=0xB0;
    tokens[begin+3]=7;
    tokens[begin+4]=11;
    tokens[begin+5]=15;

    const auto signed_magnitude = [&](std::uint32_t plane) -> std::uint16_t {
        const std::uint32_t raw=bits.read(plane);
        const std::uint32_t sign=std::uint32_t{1} << (plane-1);
        std::uint32_t magnitude=(raw & (sign-1U)) | sign;
        if ((raw & sign) != 0)
            magnitude=0U-magnitude;
        return static_cast<std::uint16_t>(magnitude);
    };
    const auto decode_four = [&](std::uint32_t first,
                                 std::uint32_t plane) {
        for (std::uint32_t i=first;i<first+4;++i) {
            if (bits.read(1) != 0) {
                tokens[--begin]=static_cast<std::uint8_t>((i<<2)|3U);
            } else {
                coefficients[i]=signed_magnitude(plane);
            }
        }
    };

    for (std::uint32_t plane=top_bitplane;plane>=1U;--plane) {
        std::uint32_t pos=begin;
        while (pos<end) {
            const std::uint8_t token=tokens[pos];
            if (token==0U || bits.read(1)==0U) {
                ++pos;
                continue;
            }
            const std::uint32_t token_index=token>>2;
            const std::uint32_t base=token & 0xFCU;
            switch (token & 3U) {
            case 0: // Four-coefficient group, now allow child expansion
                tokens[pos]=static_cast<std::uint8_t>(base+0x11U);
                decode_four(token_index,plane);
                break;
            case 1: // Split into four 4-coefficient groups
                tokens[pos]=static_cast<std::uint8_t>(base+2U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x12U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x22U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x32U);
                break;
            case 2: // Decode four coefficients, retire group
                tokens[pos]=0;
                ++pos;
                decode_four(token_index,plane);
                break;
            default: // Decode a single nonzero coefficient
                tokens[pos]=0;
                ++pos;
                coefficients[token_index]=signed_magnitude(plane);
                break;
            }
        }
    }
    // Original leaves DC coefficient at output[0:2] completely unchanged.
    for (std::uint32_t i=1;i<64;++i) {
        const auto value=coefficients[original_output_permutation[i]];
        std::memcpy(output + 2U*i,&value,sizeof(value));
    }
    bits.save();
}
