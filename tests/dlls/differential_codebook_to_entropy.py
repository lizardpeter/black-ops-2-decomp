#!/usr/bin/env python3
"""Original Bink Huffman codebook -> byte entropy chained differential.

Execute original FUN_180023840 then FUN_180024980 in sequence under Unicorn,
and independently execute our corresponding original-machine-derived C++
functions. Input Huffman *bit codes* are generated directly from the actual
original DLL's 16 codebook tables; symbol maps are initialized by the two
implementations, never injected as pretend decoder state.

Compare full output bytes, original and native initialized codebooks, and
intermediate/final bitstream cursors across every codebook type.
No FFmpeg, Bink SDK or third-party decoder implementation is used.
This covers VALID synthetic compressed streams, not real .bik frames yet.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,OBJECTS  # noqa: E402
from differential_entropy_bytes import Decoder,BitStream  # noqa: E402

PIN="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA_INIT=0x23840
RVA_BYTES=0x24980
RVA_TABLE=0x5AD40
RVA_OFFSETS=0x5A800
RVA_WIDTHS=0x5A740
WORDS=OBJECTS+0x1000
OUTPUT=OBJECTS+0x3000
WORD_COUNT=128
OUTPUT_SIZE=128

class Bits:
    def __init__(self):self.values=[]
    def push(self,value,width):
        self.values.extend((value>>i)&1 for i in range(width))
    def serialize(self,random):
        for _ in range(128):self.push(random.randrange(2),1)
        assert len(self.values)<WORD_COUNT*32
        words=[0]*WORD_COUNT
        for i,bit in enumerate(self.values):words[i//32] |= bit << (i%32)
        return struct.pack("<"+"I"*WORD_COUNT,*words)

def codes_from_real_table(img,typ):
    off=struct.unpack_from("<I",img.data,RVA_OFFSETS+4*typ)[0]
    width=img.data[RVA_WIDTHS+typ]
    lookup=img.data[RVA_TABLE+off:RVA_TABLE+off+(1<<width)]
    codes={}
    for candidate,packed in enumerate(lookup):
        code_bits=packed>>4
        symbol=packed&15
        assert 1<=code_bits<=width
        codes.setdefault(symbol,(candidate&((1<<code_bits)-1),code_bits))
    if len(codes)!=16:
        raise AssertionError(f"Pinned codebook type {typ} missing symbols: {len(codes)}")
    return codes,off,width

def header(bits,typ,mode,rng):
    bits.push(typ,4)
    if typ==0:return
    if mode=="explicit":
        prefix_count=rng.randrange(1,9)
        bits.push(1,1)
        bits.push(prefix_count-1,3)
        for symbol in rng.sample(range(16),prefix_count):
            bits.push(symbol,4)
    else:
        bits.push(0,1)
        bits.push(0,2) # 8 adjacent-pair swaps; no variable-width merges
        for _ in range(8):bits.push(rng.randrange(2),1)

def state_tuple(record,base):
    cache=struct.unpack_from("<I",record,0)[0]
    ptr=struct.unpack_from("<Q",record,8)[0]
    avail=struct.unpack_from("<I",record,16)[0]
    return cache,(ptr-base)//4,avail

def native_state_tuple(record,words):
    ptr=ctypes.cast(record.next_word,ctypes.c_void_p).value
    return record.cache,(ptr-ctypes.addressof(words))//4,record.available_bits

def main():
    original=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_codebook_chained.so"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=PIN:
        raise SystemExit("Pinned original Bink SHA-256 mismatch")
    img=Image(original)
    machine=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    init=lib.bink_init_huffman_codebook
    init.argtypes=(ctypes.POINTER(BitStream),ctypes.POINTER(ctypes.c_void_p),
                   ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8))
    init.restype=None
    decode=lib.bink_decode_entropy_bytes
    decode.argtypes=(ctypes.POINTER(Decoder),ctypes.POINTER(BitStream),
                     ctypes.POINTER(ctypes.c_uint32))
    decode.restype=None
    lib.bink_original_huffman_lookup_base.restype=ctypes.c_void_p
    native_table_base=lib.bink_original_huffman_lookup_base()

    rng=random.Random(0x23840 ^ 0x24980)
    compared=0
    for typ in range(16):
        codes,table_offset,table_width=codes_from_real_table(img,typ)
        modes=("identity",) if typ==0 else ("explicit","pair-shuffles")
        for mode in modes:
            for count in (1,2,7,16,31,47):
                for _ in range(3):
                    bits=Bits()
                    header(bits,typ,mode,rng)
                    bits.push(count,6)
                    bits.push(0,1)  # Huffman codes, not the repeated-literal branch
                    indices=[rng.randrange(16) for _ in range(count)]
                    for code in indices:
                        code_word,code_bits=codes[code]
                        bits.push(code_word,code_bits)
                    words=bits.serialize(rng)
                    fill=bytes([0xA5])*OUTPUT_SIZE
                    start=bytearray(24)
                    struct.pack_into("<Q",start,8,WORDS)
                    def setup(writer):writer.write(WORDS,words)

                    init_return,init_buffers,_,_=machine.call(img.base+RVA_INIT,
                        [bytes(start),bytes(8),bytes(4),bytes([0xA5])*16],
                        "void",setup=setup)
                    if init_return is not None:
                        raise AssertionError(f"Original codebook init fault {typ}: {init_return}")
                    orig_table=struct.unpack("<Q",init_buffers[1])[0]
                    orig_width=struct.unpack("<I",init_buffers[2])[0]
                    orig_symbols=init_buffers[3]
                    if (orig_table-img.base-RVA_TABLE,orig_width)!=(table_offset,table_width):
                        raise AssertionError(f"Wrong original codebook type {typ}")

                    original_ctx=bytearray(0x48)
                    struct.pack_into("<QQQ",original_ctx,0,OUTPUT,OUTPUT,0)
                    original_ctx[0x18:0x28]=orig_symbols
                    struct.pack_into("<I",original_ctx,0x28,orig_width)
                    struct.pack_into("<Q",original_ctx,0x30,orig_table)
                    struct.pack_into("<IIQ",original_ctx,0x38,6,63,OUTPUT)
                    def setup_decode(writer):
                        writer.write(WORDS,words)
                        writer.write(OUTPUT,fill)
                    _,decode_bufs,writes,_=machine.call(img.base+RVA_BYTES,
                        [bytes(original_ctx),init_buffers[0],WORDS+len(words)],
                        "void",setup=setup_decode)
                    original_output=b"".join(
                        writes.get(OUTPUT+i,fill[i:i+1]) for i in range(OUTPUT_SIZE))
                    independent_expect=bytes(orig_symbols[i] for i in indices)
                    if original_output[:count]!=independent_expect:
                        raise AssertionError(
                            f"Original chained codebook decode disagrees with input "
                            f"type={typ} mode={mode} count={count}")

                    native_words=(ctypes.c_uint32*WORD_COUNT)(
                        *struct.unpack("<"+"I"*WORD_COUNT,words))
                    native_bits=BitStream()
                    native_bits.next_word=ctypes.cast(native_words,
                        ctypes.POINTER(ctypes.c_uint32))
                    native_table=ctypes.c_void_p()
                    native_width=ctypes.c_uint32()
                    native_symbols=(ctypes.c_uint8*16)()
                    init(ctypes.byref(native_bits),ctypes.byref(native_table),
                         ctypes.byref(native_width),native_symbols)
                    if (native_table.value-native_table_base,native_width.value,
                        bytes(native_symbols))!=(table_offset,table_width,orig_symbols):
                        raise AssertionError(f"Chained native codebook mismatch typ={typ}")
                    if native_state_tuple(native_bits,native_words)!=state_tuple(
                            init_buffers[0],WORDS):
                        raise AssertionError(f"Intermediate codebook cursor mismatch typ={typ}")

                    native_out=(ctypes.c_uint8*OUTPUT_SIZE).from_buffer_copy(fill)
                    native_ctx=Decoder()
                    native_ctx.current=ctypes.cast(native_out,
                        ctypes.POINTER(ctypes.c_uint8))
                    native_ctx.end=native_ctx.current
                    native_ctx.symbol_map[:]=list(native_symbols)
                    native_ctx.lookup_bits=native_width.value
                    native_ctx.lookup_table=ctypes.cast(native_table,
                        ctypes.POINTER(ctypes.c_uint8))
                    native_ctx.count_bits=6
                    native_ctx.capacity=63
                    native_ctx.buffer=native_ctx.current
                    end=ctypes.cast(ctypes.byref(native_words,len(words)),
                                    ctypes.POINTER(ctypes.c_uint32))
                    decode(ctypes.byref(native_ctx),ctypes.byref(native_bits),end)
                    actual=ctypes.string_at(ctypes.addressof(native_out),OUTPUT_SIZE)
                    if actual!=original_output:
                        i=next(i for i,(a,b) in enumerate(zip(actual,original_output)) if a!=b)
                        raise AssertionError(f"Chained mismatch type={typ},mode={mode},"
                                             f"count={count},byte={i}: native={actual[i]},"
                                             f"original={original_output[i]}")
                    original_cursor=state_tuple(decode_bufs[1],WORDS)
                    native_cursor=native_state_tuple(native_bits,native_words)
                    if original_cursor!=native_cursor:
                        raise AssertionError(f"Final cursor mismatch type={typ},"
                                             f"mode={mode},count={count}: "
                                             f"original={original_cursor},native={native_cursor}")
                    original_current,original_end=struct.unpack_from("<QQ",decode_bufs[0])
                    native_base=ctypes.addressof(native_out)
                    ncur=ctypes.cast(native_ctx.current,ctypes.c_void_p).value
                    nend=ctypes.cast(native_ctx.end,ctypes.c_void_p).value
                    if (original_current-OUTPUT,original_end-OUTPUT)!=(
                            ncur-native_base,nend-native_base):
                        raise AssertionError(f"Chained decoded buffer state mismatch type={typ}")
                    compared+=1
    print(f"Original Bink codebook+entropy chained differential PASS: "
          f"{compared} synthetic streams across all 16 real original codebooks")

if __name__=="__main__":main()
