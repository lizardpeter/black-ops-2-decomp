// Original sparse pixel residual bitplanes, FUN_180030620 and
// prediction application FUN_180030A60. Transcribed from the pinned
// Store Bink x64 Ghidra output, not an external codec implementation.
// The seven-bit change budget (provided by mode 4) is decremented for
// each new coefficient or refinement; original exits on budget+1 changes.
#include "entropy_bit_cursor.h"
#include <array>
#include <cstdint>
#include <cstddef>

extern "C" void bink_video_sparse_residual(
    std::uint8_t* residual,t6_bink::WordBitStream* stream,std::uint32_t budget) {
    t6_bink::internal::BitCursor bits(*stream);
    std::uint32_t plane=bits.read(3)+1;
    std::uint32_t changes=0;
    std::array<std::uint8_t,256> tokens{};
    std::array<std::uint8_t,64> active{};
    unsigned begin=128,end=132,active_count=0;
    tokens[128]=0x10;tokens[129]=0x60;tokens[130]=0xb0;tokens[131]=2;
    for(;plane>0;--plane) {
        const auto magnitude=1U<<(plane-1);
        const auto define=[&](unsigned index) {
            active[active_count++]=static_cast<std::uint8_t>(index);
            residual[index]=static_cast<std::uint8_t>(
                bits.read(1)?0U-magnitude:magnitude);
            return changes++==budget;
        };
        const auto group=[&](unsigned index) {
            for(unsigned i=index;i<index+4;++i) {
                if(bits.read(1))
                    tokens[--begin]=static_cast<std::uint8_t>((i<<2)|3);
                else if(define(i))return true;
            }
            return false;
        };
        // First refine every coefficient introduced in EARLIER planes.
        for(unsigned i=0;i<active_count;++i) {
            if(!bits.read(1))continue;
            const auto index=active[i];
            const auto old=residual[index];
            residual[index]=static_cast<std::uint8_t>(
                old+((old&0x80U)?0U-magnitude:magnitude));
            if(changes++==budget){bits.save();return;}
        }
        unsigned pos=begin;
        while(pos<end) {
            const auto token=tokens[pos];
            if(!token||!bits.read(1)){++pos;continue;}
            const unsigned index=token>>2;
            const unsigned base=token&0xfcU;
            switch(token&3U) {
            case 0:
                tokens[pos]=static_cast<std::uint8_t>(base+0x11U);
                if(group(index)){bits.save();return;}
                break;
            case 1:
                tokens[pos]=static_cast<std::uint8_t>(base+2U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x12U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x22U);
                tokens[end++]=static_cast<std::uint8_t>(base+0x32U);
                break;
            case 2:
                tokens[pos++]=0;
                if(group(index)){bits.save();return;}
                break;
            default:
                if(define(index)){bits.save();return;}
                tokens[pos++]=0;
                break;
            }
        }
    }
    bits.save();
}

extern "C" void bink_video_residual_prediction(
    std::uint8_t* out,std::uint32_t stride,
    t6_bink::WordBitStream* bits,std::uint32_t budget,
    const std::uint8_t* reference,std::uint32_t reference_stride) {
    std::array<std::uint8_t,64> residual{};
    bink_video_sparse_residual(residual.data(),bits,budget);
    // Original 0x30A60 unrolled mapping from bitplane workspace to pixels.
    constexpr std::array<std::uint8_t,64> order={
         0, 1, 4, 5, 8, 9,12,13, 2, 3, 6, 7,10,11,14,15,
        24,25,44,45,16,17,20,21,26,27,46,47,18,19,22,23,
        28,29,32,33,48,49,52,53,30,31,34,35,50,51,54,55,
        36,37,40,41,56,57,60,61,38,39,42,43,58,59,62,63,
    };
    for(unsigned y=0;y<8;++y)
        for(unsigned x=0;x<8;++x)
            out[std::ptrdiff_t(y)*stride+x]=static_cast<std::uint8_t>(
                reference[std::ptrdiff_t(y)*reference_stride+x]+residual[order[8*y+x]]);
}
