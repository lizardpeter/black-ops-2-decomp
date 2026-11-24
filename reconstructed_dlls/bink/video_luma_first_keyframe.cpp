// Experimental first REAL BIKi keyframe luma coordinator.
// Original machine-code-derived FUN_1800252C0 control-flow translation,
// source for all ten primary dispatch modes, including reference-frame
// motion prediction, sparse residuals, and run-coded pixel blocks.
// Only the tested fixture paths are currently verified.
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
#include "video_plane.h"
#include "original_scan_orders.h"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#ifdef T6_BINK_TRACE_BITS
#include <cstdio>
#endif

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
extern "C" void bink_video_residual_prediction(std::uint8_t*,std::uint32_t,
    t6_bink::WordBitStream*,std::uint32_t,const std::uint8_t*,std::uint32_t);
extern "C" void bink_video_predict8x8(std::uint8_t*,std::int32_t,
    const std::int16_t*,std::int32_t,const std::uint8_t*,std::int32_t);
extern "C" void bink_video_fill8x8(std::uint8_t*,std::int32_t,std::uint8_t);
extern "C" void bink_video_fill16x16(std::uint8_t*,std::int32_t,std::uint8_t);
extern "C" void bink_video_two_color8x8(
    std::uint8_t*,std::int32_t,std::uint8_t,std::uint8_t,const std::uint8_t*);
extern "C" void bink_video_two_color16x16(
    std::uint8_t*,std::int32_t,std::uint8_t,std::uint8_t,const std::uint8_t*);



namespace {
// Largest Store luma row is 160 blocks; allow the original capacity plus
// alignment/lookahead tail. Each stream has independent storage.
constexpr std::uint32_t kScratchSize=16384;
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
// The plane's start and end are byte offsets into the compressed frame.
// Return the original-style next-word position in report->input_words.
// Padded 8-aligned planes up to the largest Store geometry.
// Larger and odd-sized video compatibility remains subject to differential CI.
extern "C" int bink_research_decode_video_plane_dirty(
        const std::uint8_t* compressed,std::uint32_t compressed_bytes,
        std::uint32_t start,std::uint32_t split,
        std::uint32_t width,std::uint32_t height,
        std::uint8_t* output,std::uint32_t stride,
        const std::uint8_t* reference,
        BinkFirstLumaTrace* report,std::uint8_t* dirty,std::uint32_t dirty_stride,std::uint32_t scale) {
    if (!report) return -1;
    *report=BinkFirstLumaTrace{};
    if (!compressed || !output || stride<width ||
        width<8 || height<8 || width>1280 || height>720 ||
        (width&7U)!=0U || (height&7U)!=0U ||
        compressed_bytes<4 || compressed_bytes>262148U ||
        (compressed_bytes&3U)!=0U) {
        report->status=1;return 1;
    }
    if (start>=split || split>compressed_bytes ||
        (start&3U)!=0U || (split&3U)!=0U) {
        report->status=2;return 2;
    }
    std::vector<std::uint32_t> input(compressed_bytes/4U+64U,0U);
    for (std::uint32_t i=0;i<compressed_bytes/4U;++i)
        input[i]=read_le32(compressed+4U*i);
    t6_bink::WordBitStream bits{};
    bits.next_word=input.data()+start/4;
    const auto* stream_end=input.data()+split/4;

    // The original frame-plane coordinator owns nine independent buffered
    // token streams; 32 rows of 8x8 blocks (width/8=32).
    const std::uint32_t blocks=width/8;
    std::array<std::array<std::uint8_t,kScratchSize>,kNumStreams> scratch{};
    std::array<t6_bink::ByteEntropyState,kNumStreams> stream{};
    stream[kMode]=make_stream(scratch[kMode].data(),
                            msb_width(blocks+511),align16(blocks+512),4);
    stream[kSubmode]=make_stream(scratch[kSubmode].data(),
                            msb_width(blocks/2+511),align16(blocks/2+512),4);
    stream[kColor]=make_stream(scratch[kColor].data(),
                            msb_width(blocks*64+511),blocks*64+512,8);
    stream[kPattern]=make_stream(scratch[kPattern].data(),
                            msb_width(blocks*8+511),align16(blocks*8+512),8);
    stream[kMotionX]=make_stream(scratch[kMotionX].data(),
                            msb_width(blocks+511),align16(blocks+512),5,16);
    stream[kMotionY]=make_stream(scratch[kMotionY].data(),
                            msb_width(blocks+511),align16(blocks+512),5,16);
    stream[kDC]=make_stream(scratch[kDC].data(),
                            msb_width(blocks+511),align16(blocks*2+1024),11);
    // Both original 16-bit delta contexts use a TEN-bit count width and
    // 1,088-byte capacity on this 256px plane. Original Unicorn entry-hook
    // at RVA 0x24FF0 confirms DC=(width11,mode0,count10,cap1088)
    // and otherDC=(width11,mode1024,count10,cap1088). Confusing this
    // with the separate RLE stream's 11-bit count consumes one extra bit
    // on row 0 and corrupts all subsequent coefficient transforms.
    stream[kOtherDC]=make_stream(scratch[kOtherDC].data(),
                            msb_width(blocks+511),align16(blocks*2+1024),11,1024);
    stream[kRLE]=make_stream(scratch[kRLE].data(),
                            msb_width(blocks*48+511),blocks*48+512,4);

    #ifdef T6_BINK_TRACE_BITS
    const auto trace_bits=[&](const char* phase,unsigned a,unsigned b) {
        std::fprintf(stderr,"BITTRACE %s %u %u words=%td available=%u cache=%08x\n",
            phase,a,b,bits.next_word-input.data(),bits.available_bits,bits.cache);
    };
    const auto trace_stream=[&](unsigned i,unsigned row) {
        trace_bits("entropy",row,i);
        const auto& s=stream[i];
        const auto n=s.end-s.current;
        std::fprintf(stderr,"TOKENS %u %u len=%td",row,i,n);
        if(n>0)for(std::ptrdiff_t k=0;k<n&&k<12;++k)std::fprintf(stderr," %02x",unsigned(s.current[k]));
        std::fprintf(stderr,"\n");
    };
    trace_bits("begin",width,height);
    #endif
    bink_init_huffman_codebook(&bits,&stream[kMode].lookup_table,
                &stream[kMode].lookup_bits,stream[kMode].symbol_map);
    #ifdef T6_BINK_TRACE_BITS
    trace_bits("codebook",0,0);
    #endif
    bink_init_huffman_codebook(&bits,&stream[kSubmode].lookup_table,
                &stream[kSubmode].lookup_bits,stream[kSubmode].symbol_map);
    #ifdef T6_BINK_TRACE_BITS
    trace_bits("codebook",1,0);
    #endif
    t6_bink::AdaptiveNibbleBooks books{};
    for (unsigned i=0;i<16;++i) {
        bink_init_huffman_codebook(&bits,&books.lookup_table[i],
                &books.lookup_bits[i],books.symbol_map[i]);
        #ifdef T6_BINK_TRACE_BITS
        trace_bits("adaptivebook",i,0);
        #endif
    }
    books.current_symbol=0;
    for (std::size_t i:{kColor,kPattern,kMotionX,kMotionY,kRLE}) {
        bink_init_huffman_codebook(&bits,&stream[i].lookup_table,
                  &stream[i].lookup_bits,stream[i].symbol_map);
        #ifdef T6_BINK_TRACE_BITS
        trace_bits("codebook",static_cast<unsigned>(i),0);
        #endif
    }

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
    auto consume_coeff=[&](std::uint8_t* data,std::size_t dc_stream=kDC) {
        auto& state=stream[dc_stream];
        if (state.current>=state.end) state.current=state.buffer+4;
        const auto baseline=read_le16(state.current);
        state.current+=2;
        std::memcpy(data,&baseline,2);
        #ifdef T6_BINK_TRACE_BITS
        trace_bits("coeff-before",static_cast<unsigned>(dc_stream),0);
        #endif
        bink_video_decode_coefficients(data,&bits);
        #ifdef T6_BINK_TRACE_BITS
        trace_bits("coeff-after",static_cast<unsigned>(dc_stream),0);
        #endif
    };
    auto read_quant_index=[&]() -> std::uint32_t {
        t6_bink::internal::BitCursor cursor(bits);
        const auto n=cursor.read(4);
        cursor.save();
        return n;
    };

    // Original mode 3/submode 3: four-bit scan-order selection, then
    // run lengths from the ninth entropy bundle and a repeat/literal
    // bit for each run. A final single pixel takes no extra run flag.
    const auto run_pixels=[&](std::array<std::uint8_t,64>& tile) {
        auto& runs=stream[kRLE];
        auto& colors=stream[kColor];
        if(runs.current>=runs.end)runs.current=runs.buffer+4;
        if(colors.current>=colors.end)colors.current=colors.buffer+4;
        const auto* scan=t6_bink::original_tables::scan_orders.data()+64*read_quant_index();
        unsigned pos=0;
        while(pos<63) {
            const unsigned n=unsigned(*runs.current++)+1;
            if(n>64-pos)return false;
            t6_bink::internal::BitCursor cursor(bits);
            const bool repeat=cursor.read(1)!=0;
            cursor.save();
            if(repeat) {
                const auto value=*colors.current++;
                for(unsigned j=0;j<n;++j)tile[scan[pos++]]=value;
            } else {
                for(unsigned j=0;j<n;++j)tile[scan[pos++]]=*colors.current++;
            }
        }
        if(pos==63)tile[scan[pos++]]=*colors.current++;
        return pos==64;
    };
    const auto place_pixels=[&](std::uint8_t* dst,const std::uint8_t* tile,bool doubled) {
        if(!doubled) {
            for(unsigned row=0;row<8;++row)
                std::memcpy(dst+std::ptrdiff_t(row)*stride,tile+8*row,8);
        } else {
            for(unsigned y=0;y<16;++y)
                for(unsigned x=0;x<16;++x)
                    dst[std::ptrdiff_t(y)*stride+x]=tile[8*(y/2)+x/2];
        }
    };

    // Original modes 2/7 take horizontal and vertical motion from the
    // paired signed-byte streams. The original resets both cursors when
    // the vertical stream is exhausted. The original bounds test is a
    // linear pointer comparison, not independent x/y clipping.
    const auto motion_reference=[&](std::uint32_t x,std::uint32_t y)
            -> const std::uint8_t* {
        auto& mx=stream[kMotionX];
        auto& my=stream[kMotionY];
        if (my.current>=my.end) {
            my.current=my.buffer+4;
            mx.current=mx.buffer+4;
        }
        const int dx=static_cast<std::int8_t>(*mx.current++);
        const int dy=static_cast<std::int8_t>(*my.current++);
        const auto offset=std::int64_t(y)*stride+x+dx+std::int64_t(dy)*stride;
        const auto max_offset=std::int64_t(height-8)*stride+width-8;
        if (!reference || offset<0 || offset>max_offset) return nullptr;
        return reference+offset;
    };
    const auto copy_reference=[&](std::uint8_t* dst,const std::uint8_t* src) {
        for(unsigned row=0;row<8;++row)
            std::memcpy(dst+std::ptrdiff_t(row)*stride,
                        src+std::ptrdiff_t(row)*stride,8);
    };

    const auto mark_dirty=[&](std::uint32_t x,std::uint32_t y) {
        if(dirty)dirty[std::size_t(y*scale/16)*dirty_stride+x*scale/16]=1;
    };
    if(dirty && ((scale!=1&&scale!=2)||dirty_stride<(width*scale+15)/16))return update_report(1,0,0,0,0);
    for(std::uint32_t y=0;y<height;y+=8) {
        bink_decode_entropy_byte_rle(&stream[kMode],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(0,y);
        #endif
        bink_decode_entropy_byte_rle(&stream[kSubmode],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(1,y);
        #endif
        bink_decode_adaptive_nibbles(&stream[kColor],&bits,&books,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(2,y);
        #endif
        bink_decode_entropy_nibbles(&stream[kPattern],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(3,y);
        #endif
        bink_decode_entropy_signed_bytes(&stream[kMotionX],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(4,y);
        #endif
        bink_decode_entropy_signed_bytes(&stream[kMotionY],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(5,y);
        #endif
        bink_decode_entropy_deltas16(&stream[kDC],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(6,y);
        #endif
        bink_decode_entropy_deltas16(&stream[kOtherDC],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(7,y);
        #endif
        bink_decode_entropy_bytes(&stream[kRLE],&bits,stream_end);
        #ifdef T6_BINK_TRACE_BITS
        trace_stream(8,y);
        #endif

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

        for(std::uint32_t x=0;x<width;) {
            // Original 0x252C0 dispatch guard: if the compressed next-word
            // pointer reaches the supplied end OR the mode stream is
            // exhausted, take the default 8-pixel advance, without
            // reading a mode or touching destination pixels. This matters
            // for the two final 8x8 blocks of the real second chroma plane.
            if (bits.next_word>=stream_end ||
                stream[kMode].current>=stream[kMode].end) {
                x+=8;
                continue;
            }
            const std::uint32_t mode=*stream[kMode].current++;
            #ifdef T6_BINK_TRACE_BITS
            trace_bits("mode",y,(x<<8)|mode);
            #endif
            if(mode<10U)++report->type_counts[mode];
            else return update_report(4,x,y,mode,0xff);
            std::uint8_t* block=output+std::ptrdiff_t(y)*stride+x;
            // Original non-copy primary modes set the corresponding byte
            // in the macroblock dirty map; the doubled mode is handled below.
            if(mode!=0U && mode!=1U)mark_dirty(x,y);
            if (mode==0U) {
                if (!reference) return update_report(7,x,y,mode,0xff);
                copy_reference(block,reference+std::ptrdiff_t(y)*stride+x);
                x+=8;
            } else if (mode==2U || mode==4U || mode==7U) {
                if (!reference) return update_report(7,x,y,mode,0xff);
                const auto* src=motion_reference(x,y);
                if (src && mode==2U) copy_reference(block,src);
                else if(src && mode==4U) {
                    t6_bink::internal::BitCursor cursor(bits);
                    const auto budget=cursor.read(7);
                    cursor.save();
                    bink_video_residual_prediction(block,stride,&bits,budget,src,stride);
                } else if (src) {
                    alignas(2) std::array<std::uint8_t,128> coeff{};
                    consume_coeff(coeff.data(),kOtherDC);
                    bink_video_predict8x8(block,std::int32_t(stride),
                        reinterpret_cast<const std::int16_t*>(coeff.data()),
                        std::int32_t(read_quant_index()),src,std::int32_t(stride));
                }
                x+=8;
            } else if (mode==3U) {
                std::array<std::uint8_t,64> tile{};
                if(!run_pixels(tile))return update_report(8,x,y,mode,0xff);
                place_pixels(block,tile.data(),false);
                x+=8;
            } else if (mode==9U) {
                auto& state=stream[kColor];
                if (state.current>=state.end) state.current=state.buffer+4;
                for(unsigned row=0;row<8;++row)
                    std::memcpy(block+std::ptrdiff_t(row)*stride,state.current+8*row,8);
                state.current+=64;
                x+=8;
            } else if(mode==1U) {
                // At the right edge the original breaks out of the mode-1
                // case and advances 8 pixels instead of 16.
                if (x+16>width) { x+=8; continue; }
                mark_dirty(x,y);mark_dirty(x+8,y);
                std::uint32_t subtype=0xff;
                if ((y&8U)==0 && x+16<=width &&
                    stream[kSubmode].current<stream[kSubmode].end) {
                    subtype=*stream[kSubmode].current++;
                    #ifdef T6_BINK_TRACE_BITS
                    trace_bits("submode",y,(x<<8)|subtype);
                    #endif
                    if(subtype<10U)++report->submode_counts[subtype];
                    else return update_report(5,x,y,mode,subtype);
                    // Do not write a 16x16 block beyond the allocated plane.
                    // A valid stream should not dispatch one on an 8-row tail.
                    if(y+16>height)return update_report(9,x,y,mode,subtype);
                    if(subtype==3U) {
                        std::array<std::uint8_t,64> tile{};
                        if(!run_pixels(tile))return update_report(8,x,y,mode,subtype);
                        place_pixels(block,tile.data(),true);
                    } else if(subtype==9U) {
                        auto& colors=stream[kColor];
                        if(colors.current>=colors.end)colors.current=colors.buffer+4;
                        place_pixels(block,colors.current,true);
                        colors.current+=64;
                    } else if(subtype==6U) {
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
    return update_report(0,width,height,0,0);
}

extern "C" int bink_research_decode_video_plane(
    const std::uint8_t* compressed,std::uint32_t compressed_bytes,std::uint32_t start,std::uint32_t split,
    std::uint32_t width,std::uint32_t height,std::uint8_t* output,std::uint32_t stride,
    const std::uint8_t* reference,BinkFirstLumaTrace* report) {
    return bink_research_decode_video_plane_dirty(compressed,compressed_bytes,start,split,width,height,
        output,stride,reference,report,nullptr,0,1);
}

// Retain original keyframe-only API for existing callers.
extern "C" int bink_research_decode_keyframe_plane(
        const std::uint8_t* compressed,std::uint32_t compressed_bytes,
        std::uint32_t start,std::uint32_t split,
        std::uint32_t width,std::uint32_t height,
        std::uint8_t* output,std::uint32_t stride,
        BinkFirstLumaTrace* report) {
    return bink_research_decode_video_plane(compressed,compressed_bytes,
        start,split,width,height,output,stride,nullptr,report);
}

// Preserve the original tested first-luma entry point and packet layout.
extern "C" int bink_research_decode_boi_first_luma(
        const std::uint8_t* compressed,std::uint32_t compressed_bytes,
        std::uint8_t* output,std::uint32_t stride,
        BinkFirstLumaTrace* report) {
    if (!report) return -1;
    if (!compressed || compressed_bytes<8) {
        *report=BinkFirstLumaTrace{};
        report->status=1;
        return 1;
    }
    return bink_research_decode_keyframe_plane(compressed,compressed_bytes,
        4,read_le32(compressed),256,256,output,stride,report);
}
