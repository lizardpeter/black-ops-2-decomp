// Unit tests for original Bink signed-byte Huffman decoder at 0x24DB0.
// Pinned-original differential lives in tests/dlls/differential_entropy_signed.py.
#include "entropy_signed_bytes.cpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

static void pack(std::array<std::uint32_t,16>& words, std::size_t& index,
                 std::uint32_t value, unsigned width) {
    for (unsigned b=0;b<width;++b,++index)
        if (value & (1U << b)) words[index/32] |= 1U << (index%32);
}
int main() {
    std::uint8_t table[16];
    for (unsigned i=0;i<16;++i) table[i] = std::uint8_t(0x40U | i);

    // 4 Huffman bytes: +1, -2, +3, -4.
    std::array<std::uint32_t,16> words{};
    std::size_t idx=0;
    pack(words,idx,4,6); pack(words,idx,0,1);
    for (unsigned v=1;v<=4;++v) {
        pack(words,idx,v,4);
        pack(words,idx,v%2==0,1);
    }
    std::array<std::uint8_t,64> storage{};
    storage.fill(0xA5);
    t6_bink::ByteEntropyState ctx{
        storage.data(),storage.data(),0,{},4,0,table,6,63,storage.data()};
    for (unsigned i=0;i<16;++i) ctx.symbol_map[i] = std::uint8_t(i);
    t6_bink::WordBitStream bits{0,0,words.data(),0,0};
    bink_decode_entropy_signed_bytes(&ctx,&bits,words.data()+words.size());
    assert(ctx.current == storage.data() && ctx.end == storage.data()+4);
    assert(storage[0]==1 && storage[1]==254);
    assert(storage[2]==3 && storage[3]==252);
    for (unsigned i=4;i<64;++i) assert(storage[i]==0xA5);

    // Repeated signed nibble (-5) uses a single nibble and a sign bit.
    words.fill(0);
    idx=0; pack(words,idx,3,6); pack(words,idx,1,1);
    pack(words,idx,5,4); pack(words,idx,1,1);
    storage.fill(0xA5);
    ctx.current=ctx.end=storage.data();
    bits.cache=0; bits.available_bits=0; bits.next_word=words.data();
    bink_decode_entropy_signed_bytes(&ctx,&bits,words.data()+words.size());
    assert(ctx.end==storage.data()+3);
    for (unsigned i=0;i<3;++i) assert(storage[i]==251);
    for (unsigned i=3;i<64;++i) assert(storage[i]==0xA5);
}
