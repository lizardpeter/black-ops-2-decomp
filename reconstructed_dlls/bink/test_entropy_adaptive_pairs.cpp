// Original Bink adaptive nibble pair source tests (0x243B0/0x246A0).
// Binary differential: tests/dlls/differential_entropy_adaptive_pairs.py.
#include "entropy_adaptive_pairs.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

struct Packer {
    std::array<std::uint32_t,64> words{};
    std::size_t pos=0;
    void push(unsigned value,unsigned n) {
        for(unsigned i=0;i<n;++i,++pos)
            if ((value>>i)&1U) words[pos/32] |= 1U<<(pos%32);
    }
};

static void verify(bool signed_mode,bool repeat) {
    std::uint8_t lookup[16]{};
    for(unsigned i=0;i<16;++i)lookup[i]=std::uint8_t(0x40|i);
    t6_bink::AdaptiveNibbleBooks books{};
    for(unsigned b=0;b<16;++b) {
        books.lookup_bits[b]=4;
        books.lookup_table[b]=lookup;
        for(unsigned j=0;j<16;++j)
            books.symbol_map[b][j]=std::uint8_t(j);
    }
    books.current_symbol=0;
    std::array<std::uint8_t,64> out{};
    out.fill(0xA5);
    t6_bink::ByteEntropyState state{
        out.data(),out.data(),0,{},4,0,lookup,6,63,out.data()
    };
    for(unsigned i=0;i<16;++i)state.symbol_map[i]=std::uint8_t(i);
    Packer p;
    const unsigned count=repeat?9:5;
    p.push(count,6);p.push(unsigned(repeat),1);
    if(repeat) {
        p.push(12,4);p.push(7,4);
    } else {
        for(unsigned i=0;i<count;++i) {
            p.push(i+1,4);
            p.push((i+2)&15,4);
        }
    }
    t6_bink::WordBitStream bits{0,0,p.words.data(),0,0};
    if(signed_mode)
        bink_decode_adaptive_nibbles_signed(&state,&bits,&books,
                                             p.words.data()+p.words.size());
    else
        bink_decode_adaptive_nibbles(&state,&bits,&books,
                                     p.words.data()+p.words.size());
    assert(state.current==out.data() && state.end==out.data()+count);
    for(unsigned i=0;i<count;++i) {
        const std::uint8_t packed=repeat?0xC7U:
            std::uint8_t(((i+1)<<4)|((i+2)&15));
        const std::uint8_t magnitude=packed&0x7FU;
        const int shifted=(packed&0x80U)?-int(magnitude):int(magnitude);
        const std::uint8_t expected=signed_mode?
            std::uint8_t(shifted+0x80):packed;
        assert(out[i]==expected);
    }
    for(unsigned i=count;i<out.size();++i)assert(out[i]==0xA5);
    assert(books.current_symbol==(repeat?12U:5U));
}

int main() {
    static_assert(sizeof(t6_bink::AdaptiveNibbleBooks)==0x1C8);
    for(bool signed_mode:{false,true})
        for(bool repeat:{false,true})
            verify(signed_mode,repeat);
}
