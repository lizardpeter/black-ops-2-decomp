// Standalone unit tests for Bink original 0x24140 Huffman byte/RLE decoder.
// Original-vs-source comparisons: tests/dlls/differential_entropy_byte_rle.py
#include "entropy_byte_rle.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {
struct Packed {
    std::array<std::uint32_t, 32> words{};
    std::size_t pos = 0;
    void put(std::uint32_t value, unsigned bits) {
        for (unsigned i=0; i<bits; ++i, ++pos)
            if ((value >> i) & 1U)
                words[pos / 32] |= 1U << (pos % 32);
    }
};

void check(Packed p, const std::vector<std::uint8_t>& expected) {
    std::array<std::uint8_t, 64> out{};
    out.fill(0xA5);
    std::uint8_t table[16];
    for (unsigned i=0;i<16;++i) table[i] = std::uint8_t(0x40U | i);
    t6_bink::ByteEntropyState state{
        out.data(), out.data(), 0, {}, 4, 0, table, 6, 63, out.data()
    };
    for (unsigned i=0;i<16;++i) state.symbol_map[i]=std::uint8_t(i);
    t6_bink::WordBitStream bits{0,0,p.words.data(),0,0};
    bink_decode_entropy_byte_rle(&state,&bits,p.words.data()+p.words.size());
    assert(state.current==out.data() && state.end==out.data()+expected.size());
    for (std::size_t i=0;i<expected.size();++i) assert(out[i]==expected[i]);
    for (std::size_t i=expected.size();i<out.size();++i) assert(out[i]==0xA5);
}
}

int main() {
    {
        Packed p;
        p.put(18,6); p.put(0,1);
        for (unsigned sym : {2U, 12U, 7U, 13U, 4U, 5U, 6U, 7U})
            p.put(sym,4);
        std::vector<std::uint8_t> expected(5,2);
        expected.insert(expected.end(),9,7);
        for (unsigned value : {4U,5U,6U,7U})
            expected.push_back(static_cast<std::uint8_t>(value));
        assert(expected.size()==18);
        check(p,expected);
    }
    {
        Packed p;
        p.put(48,6); p.put(0,1);
        for (unsigned sym : {3U,15U,14U,9U,10U,11U}) p.put(sym,4);
        std::vector<std::uint8_t> expected(45,3);
        expected.push_back(9);
        expected.push_back(10);
        expected.push_back(11);
        check(p,expected);
    }
    for (unsigned count : {1U,7U,31U,63U}) {
        Packed p;
        p.put(count,6); p.put(1,1); p.put(0xBU,4);
        check(p,std::vector<std::uint8_t>(count,11));
    }
    {
        // Original error-pointer sentinel when count=0.
        Packed p; p.put(0,6);
        std::array<std::uint8_t,64> out{};
        std::uint8_t table[16]{};
        t6_bink::ByteEntropyState d{
            out.data(),out.data(),0,{},4,0,table,6,63,out.data()
        };
        t6_bink::WordBitStream bits{0,0,p.words.data(),0,0};
        bink_decode_entropy_byte_rle(&d,&bits,p.words.data()+p.words.size());
        assert(d.current==out.data()+4 && d.end==out.data());
    }
}
