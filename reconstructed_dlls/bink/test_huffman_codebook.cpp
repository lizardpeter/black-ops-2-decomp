// Standalone tests for source translation of original Bink RVA 0x23840.
// Exact original x64-versus-native comparison: differential_huffman_codebook.py.
#include "huffman_codebook.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

struct Packed {
    std::array<std::uint32_t,16> words{};
    std::uint32_t pos=0;
    void push(std::uint32_t value, std::uint32_t bits) {
        for (std::uint32_t i=0;i<bits;++i,++pos)
            if ((value>>i)&1U) words[pos/32] |= 1U<<(pos%32);
    }
};

static std::array<std::uint8_t,16> run(Packed& p,std::uint32_t expected_width,
                                      std::uint32_t table_offset, bool exact_bits=true) {
    t6_bink::WordBitStream stream{0,0,p.words.data(),0,0};
    const std::uint8_t* table=nullptr;
    std::uint32_t width=0;
    std::array<std::uint8_t,16> map{};
    bink_init_huffman_codebook(&stream,&table,&width,map.data());
    assert(width==expected_width);
    assert(table==bink_original_huffman_lookup_base()+table_offset);
    std::array<bool,16> seen{};
    for (auto value:map) {
        assert(value<16 && !seen[value]);
        seen[value]=true;
    }
    const std::uint32_t expected_avail=(32U-(p.pos%32U))%32U;
    if (exact_bits) assert(stream.available_bits==expected_avail);
    return map;
}

int main() {
    static_assert(t6_bink::original_tables::packed_lookups.size()==976);
    // Codebook type zero ignores the permutation suffix entirely.
    {
        Packed p; p.push(0,4);
        const auto map=run(p,4,0);
        for (unsigned i=0;i<16;++i) assert(map[i]==i);
    }
    // Original's explicit-front permutation with remaining unused symbols
    // in ascending order. Type 1 uses 5-bit table at original offset 16.
    {
        Packed p;
        p.push(1,4);p.push(1,1);p.push(3,3);
        for (auto v:{5U,7U,1U,14U})p.push(v,4);
        const auto map=run(p,5,16);
        constexpr std::array<std::uint8_t,16> expected={
            5,7,1,14,0,2,3,4,6,8,9,10,11,12,13,15
        };
        assert(map==expected);
    }
    // Depth zero: exactly eight 1-bit adjacent-pair swaps.
    {
        Packed p;p.push(7,4);p.push(0,1);p.push(0,2);
        for(unsigned i=0;i<8;++i)p.push(i%2,1);
        const auto map=run(p,6,208);
        for(unsigned i=0;i<8;++i) {
            assert(map[2*i]==2*i+(i%2));
            assert(map[2*i+1]==2*i+((i%2)^1));
        }
    }
    // Run each deeper stable-merge hierarchy; unique permutation is checked.
    for(std::uint32_t depth=1;depth<=3;++depth) {
        Packed p;p.push(15,4);p.push(0,1);p.push(depth,2);
        for(unsigned i=0;i<8;++i)p.push((i*3)&1,1);
        for(unsigned stage=1;stage<=depth;++stage)
            for(unsigned block=0;block<16/(1U<<(stage+1));++block)
                p.push(0x5A5A, (1U<<(stage+1))-1);
        (void)run(p,7,848,false);
    }
}
