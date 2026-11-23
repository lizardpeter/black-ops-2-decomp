// Source-level smoke tests of original Bink coefficient bit-plane decoder.
// Full pinned-original 65-block differential: differential_real_coefficients.py.
#include "video_coefficients.cpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

static void run(std::uint32_t encoded, std::uint16_t expected_ac1) {
    std::array<std::uint32_t,32> input{};
    input[0]=encoded;
    std::array<std::uint8_t,128> output{};
    output.fill(0xA5);
    const std::uint16_t dc=0x3412;
    std::memcpy(output.data(),&dc,sizeof(dc));
    t6_bink::WordBitStream bits{0,0,input.data(),0,0};
    bink_video_decode_coefficients(output.data(),&bits);
    assert(output[0]==0x12 && output[1]==0x34);
    std::uint16_t ac1=0;
    std::memcpy(&ac1,output.data()+2,2);
    assert(ac1==expected_ac1);
    for(unsigned i=2;i<64;++i) {
        assert(output[2*i]==0 && output[2*i+1]==0);
    }
}
int main() {
    // Zero bitplane: no token significance bits required; DC unchanged.
    run(0,0);
    // Bitplane 1: six zero token significance bits, all AC coefficients 0.
    run(1,0);
    // Bitplane 1; initial 3 group tokens 0, first single token significant:
    // first AC is +1 when sign bit is 0, -1 when sign bit is 1.
    // Encoding: plane=1 at bits[0:4]; initial three flags 0; next flag 1.
    run(1U|(1U<<7),1);
    run(1U|(1U<<7)|(1U<<8),0xFFFF);
}
