// Experimental first REAL BIKi keyframe luma coordinator.
// Original machine-code-derived FUN_1800252C0 control-flow translation,
// restricted to the block modes OBSERVED on the first original BO2 video
// la_pip_seq_6.bik (256x256): 1/submode(5,6,8), 5, 6, and 8.
//
// Unlike earlier original-only oracles this attempts the complete LUMA plane
// using ONLY independently reconstructed C++ source routines, original
// immutable Huffman/quantization tables, and the actual compressed packet.
// It does NOT call into the original DLL and uses no codec replacement.
// This is an isolated research decoder, NOT yet integrated into BO2 and
// NOT a complete Bink audio/color/animation decoder.
//
// Original Store binkw64.dll SHA256:
// 10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17
#include "entropy_bit_cursor.h"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace t6_bink {
struct AdaptiveNibbleBooks {
    std::uint8_t symbol_map[16][16];
    std::uint32_t lookup_bits[16];
    const std::uint8_t* lookup_table[16];
    std::uint32_t current_symbol;
};
static_assert(offsetof(AdaptiveNibbleBooks,current_symbol)==0x1c0);
}

extern "C" void bink_init_huffman_codebook(
    t6_bink::WordBitStream*,const std::uint8_t**,std::uint32_t*,std::uint8_t*);
extern "C" void bink_decode_entropy_byte_rle(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,const std::uint32_t*);
extern "C" void bink_decode_adaptive_nibbles(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,
    t6_bink::AdaptiveNibbleBooks*,const std::uint32_t*);
extern "C" void bink_decode_entropy_nibbles(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,const std::uint32_t*);
extern "C" void bink_decode_entropy_signed_bytes(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,const std::uint32_t*);
extern "C" void bink_decode_entropy_deltas16(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,const std::uint32_t*);
extern "C" void bink_decode_entropy_bytes(
    t6_bink::ByteEntropyState*,t6_bink::WordBitStream*,const std::uint32_t*);
extern "C" void bink_video_decode_coefficients(
    std::uint8_t*,t6_bink::WordBitStream*);
extern "C" void bink_video_inverse8x8_indexed(
    std::uint8_t*,std::int32_t,const std::int16_t*,std::uint32_t);
extern "C" void bink_video_inverse8x8_2x_indexed(
    std::uint8_t*,std::int32_t,const std::int16_t*,std::uint32_t);
extern "C" void bink_video_fill8x8(std::uint8_t*,std::int32_t,std::uint8_t);
extern "C" void bink_video_fill16x16(std::uint8_t*,std::int32_t,std::uint8_t);
extern "C" void bink_video_two_color8x8(
    std::uint8_t*,std::int32_t,std::uint8_t,std::uint8_t,const std::uint8_t*);
extern "C" void bink_video_two_color16x16(
    std::uint8_t*,std::int32_t,std::uint8_t,std::uint8_t,const std::uint8_t*);

struct BinkFirstLumaTrace {
    std::uint32_t status;
    std::uint32_t error_x;
    std::uint32_t error_y;
    std::uint32_t mode;
    std::uint32_t submode;
    std::uint32_t type_counts[10];
    std::uint32_t submode_counts[10];
    std::uint32_t input_words;
    std::uint32_t input_bits_available;
    std::uint32_t input_cache;
    std::uint32_t first_mode_count;
    std::uint8_t first_mode_bytes[32];
    std::uint32_t first_submode_count;
    std::uint8_t first_submode_bytes[32];
};

namespace {
constexpr std::uint32_t kWidth=256,kHeight=256,kScratchSize=8192;
enum :std::size_t {
    kMode=0,kSubmode=1,kColor=2,kPattern=3,
    kMotionX=4,kMotionY=5,kDC=6,kOtherDC=7,kRLE=8,kNumStreams=9
};
std::uint32_t msb_width(std::uint32_t n) {
    return std::bit_width(n);
}
std::uint32_t align16(std::uint32_t n) {
    return (n+15U)&~15U;
}
std::uint32_t read_le32(const std::uint8_t* p) {
    return std::uint32_t(p[0]) |
        (std::uint32_t(p[1])<<8) |
        (std::uint32_t(p[2])<<16) |
        (std::uint32_t(p[3])<<24);
}
std::uint16_t read_le16(const std::uint8_t* p) {
    return std::uint16_t(p[0]) |
        std::uint16_t(std::uint16_t(p[1])<<8);
}
t6_bink::ByteEntropyState make_stream(
        std::uint8_t* buffer,std::uint32_t count_bits,
        std::uint32_t capacity,std::uint32_t base_width,
        std::uint32_t sign_mode=0) {
    t6_bink::ByteEntropyState ctx{};
    ctx.reserved10=std::uint64_t(base_width) |
        (std::uint64_t(sign_mode)<<32);
    ctx.count_bits=count_bits;
    ctx.capacity=capacity;
    ctx.buffer=buffer;
    return ctx;
}
} // namespace

// Status 0 is a fully completed luma output. Nonzero is an explicit
// diagnostic failure; no completion should be claimed in that case.
extern "C" int bink_research_decode_boi_first_luma(
        const std::uint8_t* compressed,std::uint32_t compressed_bytes,
        std::uint8_t* output,std::uint32_t stride,
        BinkFirstLumaTrace* report) {
    if (!report) return -1;
    *report=BinkFirstLumaTrace{};
    if (!compressed || !output || stride<kWidth ||
        compressed_bytes<8 || compressed_bytes>32768U ||
        (compressed_bytes&3U)!=0U) {
        report->status=1;return 1;
    }
    const std::uint32_t split=read_le32(compressed);
    if (split<8 || split>compressed_bytes || (split&3U)!=0U) {
        report->status=2;return 2;
    }
    std::vector<std::uint32_t> input(compressed_bytes/4U+64U,0U);
    for (std::uint32_t i=0;i<compressed_bytes/4U;++i)
        input[i]=read_le32(compressed+4U*i);
    t6_bink::WordBitStream bits{};
    bits.next_word=input.data()+1;
    const auto* stream_end=input.data()+split/4;

    // The original frame-plane coordinator owns nine independent buffered
    // token streams; 32 rows of 8x8 blocks (width/8=32).
    constexpr std::uint32_t blocks=32;
    std::array<std::array<std::uint8_t,kScratchSize>,kNumStreams> scratch{};
    std::array<t6_bink::ByteEntropyState,kNumStreams> stream{};
    stream[kMode]=make_stream(scratch[kMode].data(),
                            msb_width(blocks+511),align16(blocks+527),4);
    stream[kSubmode]=make_stream(scratch[kSubmode].data(),
                            msb_width(blocks/2+511),align16(blocks/2+527),4);
    stream[kColor]=make_stream(scratch[kColor].data(),
                            msb_width(blocks*64+511),blocks*64+512,8);
    stream[kPattern]=make_stream(scratch[kPattern].data(),
                            msb_width(blocks*8+511),align16(blocks*8+527),8);
    stream[kMotionX]=make_stream(scratch[kMotionX].data(),
                            msb_width(blocks+511),align16(blocks+527),5,16);
    stream[kMotionY]=make_stream(scratch[kMotionY].data(),
                            msb_width(blocks+511),align16(blocks+527),5,16);
    stream[kDC]=make_stream(scratch[kDC].data(),
                            msb_width(blocks+511),align16(blocks*2+1039),11);
    // Both original 16-bit delta contexts use a TEN-bit count width and
    // 1,088-byte capacity on this 256px plane. Original Unicorn entry-hook
    // at RVA 0x24FF0 confirms DC=(width11,mode0,count10,cap1088)
    // and otherDC=(width11,mode1024,count10,cap1088). Confusing this
    // with the separate RLE stream's 11-bit count consumes one extra bit
    // on row 0 and corrupts all subsequent coefficient transforms.
    stream[kOtherDC]=make_stream(scratch[kOtherDC].data(),
                            msb_width(blocks+511),align16(blocks*2+1039),11,1024);
    stream[kRLE]=make_stream(scratch[kRLE].data(),
                            msb_width(blocks*48+511),blocks*48+512,4);

    bink_init_huffman_codebook(&bits,&stream[kMode].lookup_table,
                &stream[kMode].lookup_bits,stream[kMode].symbol_map);
    bink_init_huffman_codebook(&bits,&stream[kSubmode].lookup_table,
                &stream[kSubmode].lookup_bits,stream[kSubmode].symbol_map);
    t6_bink::AdaptiveNibbleBooks books{};
    for (unsigned i=0;i<16;++i)
        bink_init_huffman_codebook(&bits,&books.lookup_table[i],
                &books.lookup_bits[i],books.symbol_map[i]);
    books.current_symbol=0;
    for (std::size_t i:{kColor,kPattern,kMotionX,kMotionY,kRLE})
        bink_init_huffman_codebook(&bits,&stream[i].lookup_table,
                  &stream[i].lookup_bits,stream[i].symbol_map);

    const auto update_report=[&](int code,std::uint32_t x,std::uint32_t y,
                                  std::uint32_t mode,std::uint32_t submode) {
        report->status=std::uint32_t(code);
        report->error_x=x;
        report->error_y=y;
        report->mode=mode;
        report->submode=submode;
        report->input_words=std::uint32_t(bits.next_word-input.data());
        report->input_bits_available=bits.available_bits;
        report->input_cache=bits.cache;
        return code;
    };

    // Original FUN_1800252C0 uses a *byte* pointer under its decompiler's
    // ushort* color-stream alias: case 6 and submode 6 advance by ONE byte
    // (cast pointer to integer, add 1), whereas both two-color modes 8
    // advance by TWO bytes (puVar24 + 1 as ushort*).
    // Advancing by two for solid fills exhausts the initial color block
    // early and desynchronizes every subsequent entropy refill.
    auto consume_color=[&](std::uint32_t bytes) -> std::uint16_t {
        auto& state=stream[kColor];
        if (state.current>=state.end) state.current=state.buffer+4;
        const std::uint16_t color=read_le16(state.current);
        state.current+=bytes;
        return color;
    };
    auto consume_pattern=[&]() -> const std::uint8_t* {
        auto& state=stream[kPattern];
        if (state.current>=state.end) state.current=state.buffer+4;
        const auto* bytes=state.current;
        state.current+=8;
        return bytes;
    };
    auto consume_coeff=[&](std::uint8_t* data) {
        auto& state=stream[kDC];
        if (state.current>=state.end) state.current=state.buffer+4;
        const auto baseline=read_le16(state.current);
        state.current+=2;
        std::memcpy(data,&baseline,2);
        bink_video_decode_coefficients(data,&bits);
    };
    auto read_quant_index=[&]() -> std::uint32_t {
        t6_bink::internal::BitCursor cursor(bits);
        const auto n=cursor.read(4);
        cursor.save();
        return n;
    };

    for(std::uint32_t y=0;y<kHeight;y+=8) {
        bink_decode_entropy_byte_rle(&stream[kMode],&bits,stream_end);
        bink_decode_entropy_byte_rle(&stream[kSubmode],&bits,stream_end);
        bink_decode_adaptive_nibbles(&stream[kColor],&bits,&books,stream_end);
        bink_decode_entropy_nibbles(&stream[kPattern],&bits,stream_end);
        bink_decode_entropy_signed_bytes(&stream[kMotionX],&bits,stream_end);
        bink_decode_entropy_signed_bytes(&stream[kMotionY],&bits,stream_end);
        bink_decode_entropy_deltas16(&stream[kDC],&bits,stream_end);
        bink_decode_entropy_deltas16(&stream[kOtherDC],&bits,stream_end);
        bink_decode_entropy_bytes(&stream[kRLE],&bits,stream_end);

        if (y==0) {
            const auto n=stream[kMode].end-stream[kMode].current;
            const auto n2=stream[kSubmode].end-stream[kSubmode].current;
            if(n>=0&&n<65536) {
                report->first_mode_count=std::uint32_t(n);
                for(std::uint32_t i=0;i<32&&i<std::uint32_t(n);++i)
                    report->first_mode_bytes[i]=stream[kMode].current[i];
            }
            if(n2>=0&&n2<65536) {
                report->first_submode_count=std::uint32_t(n2);
                for(std::uint32_t i=0;i<32&&i<std::uint32_t(n2);++i)
                    report->first_submode_bytes[i]=stream[kSubmode].current[i];
            }
        }

        for(std::uint32_t x=0;x<kWidth;) {
            if (stream[kMode].current>=stream[kMode].end)
                return update_report(3,x,y,0xff,0xff);
            const std::uint32_t mode=*stream[kMode].current++;
            if(mode<10U)++report->type_counts[mode];
            else return update_report(4,x,y,mode,0xff);
            std::uint8_t* block=output+std::ptrdiff_t(y)*stride+x;
            if(mode==1U) {
                std::uint32_t subtype=0xff;
                if ((y&8U)==0 && x+16<=kWidth &&
                    stream[kSubmode].current<stream[kSubmode].end) {
                    subtype=*stream[kSubmode].current++;
                    if(subtype<10U)++report->submode_counts[subtype];
                    else return update_report(5,x,y,mode,subtype);
                    if(subtype==6U) {
                        bink_video_fill16x16(block,std::int32_t(stride),
                                             std::uint8_t(consume_color(1)));
                    } else if (subtype==8U) {
                        const auto colors=consume_color(2);
                        bink_video_two_color16x16(block,std::int32_t(stride),
                            std::uint8_t(colors),std::uint8_t(colors>>8),
                            consume_pattern());
                    } else if (subtype==5U) {
                        alignas(2) std::array<std::uint8_t,128> coeff{};
                        consume_coeff(coeff.data());
                        bink_video_inverse8x8_2x_indexed(
                            block,std::int32_t(stride),
                            reinterpret_cast<const std::int16_t*>(coeff.data()),
                            read_quant_index());
                    } else {
                        return update_report(6,x,y,mode,subtype);
                    }
                }
                x+=16;
            } else if(mode==5U) {
                alignas(2) std::array<std::uint8_t,128> coeff{};
                consume_coeff(coeff.data());
                bink_video_inverse8x8_indexed(
                    block,std::int32_t(stride),
                    reinterpret_cast<const std::int16_t*>(coeff.data()),
                    read_quant_index());
                x+=8;
            } else if(mode==6U) {
                bink_video_fill8x8(block,std::int32_t(stride),
                                   std::uint8_t(consume_color(1)));
                x+=8;
            } else if(mode==8U) {
                const auto colors=consume_color(2);
                bink_video_two_color8x8(block,std::int32_t(stride),
                    std::uint8_t(colors),std::uint8_t(colors>>8),
                    consume_pattern());
                x+=8;
            } else {
                return update_report(7,x,y,mode,0xff);
            }
        }
    }
    return update_report(0,kWidth,kHeight,0,0);
}
