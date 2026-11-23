#!/usr/bin/env python3
"""Differential original Bink adaptive Huffman pair decoders vs C++ source.

FUN_1800246A0 and FUN_1800243B0 are run on SHA-pinned original Store
binkw64.dll instructions in Unicorn. Our separately compiled C++ routines
operate on equivalent original-layout 0x48 decoder, 0x18 bitreader and 0x1C8
adaptive-Huffman records. Checks full output, current adaptive symbol index,
output buffer pointers and exact bit-consumption state. Fixed 4-bit original
Huffman table semantics with dynamically changing high-nibble codebooks,
repeat-single-symbol mode and word-boundary crossings are covered.
No codec replacement or real full-frame source decode is claimed.
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
WORDS,LOOKUPS,OUTPUT=OBJECTS+0x1000,OBJECTS+0x2000,OBJECTS+0x3000
LOW_TABLE=LOOKUPS+0x400
WORD_COUNT=128
OUTPUT_BYTES=128
ROUTINES=(
    (0x246A0,"bink_decode_adaptive_nibbles",False),
    (0x243B0,"bink_decode_adaptive_nibbles_signed",True),
)

class Adaptive(ctypes.Structure):
    _fields_=(("symbol_map",(ctypes.c_uint8*16)*16),
              ("lookup_bits",ctypes.c_uint32*16),
              ("lookup_table",ctypes.c_void_p*16),
              ("current_symbol",ctypes.c_uint32))
assert ctypes.sizeof(Adaptive)==0x1C8

def stream_for(rng,count,repeat,first_book):
    bits=[]
    def put(value,width):
        bits.extend((value>>i)&1 for i in range(width))
    put(count,6)
    expected=[]
    current=first_book
    if count:
        put(int(repeat),1)
        for i in range(1 if repeat else count):
            high_code=rng.randrange(16)
            low_code=rng.randrange(16)
            put(high_code,4)
            put(low_code,4)
            high=(high_code+current)&15
            current=high
            expected.append((high<<4)|low_code)
        if repeat:
            expected *= count
    words=[0]*WORD_COUNT
    for i,bit in enumerate(bits):
        words[i//32] |= bit << (i%32)
    return struct.pack("<"+"I"*WORD_COUNT,*words),expected,current

def main():
    original=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_entropy_adaptive_pairs.so"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=PIN:
        raise SystemExit("Original Store Bink hash mismatch")
    img=Image(original)
    emulator=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    for _,name,_ in ROUTINES:
        f=getattr(lib,name)
        f.argtypes=(ctypes.POINTER(Decoder),ctypes.POINTER(BitStream),
                    ctypes.POINTER(Adaptive),ctypes.POINTER(ctypes.c_uint32))
        f.restype=None

    rng=random.Random(0x243B0 ^ 0x246A0)
    cases=0
    fixed=bytes(0x40|i for i in range(16))
    lowmap=bytes(range(16))
    for rva,name,signed in ROUTINES:
        fn=getattr(lib,name)
        for count in (0,1,2,3,7,8,9,15,16,31,32,47,63):
            for repeat in ((False,True) if count else (False,)):
                for trial in range(12):
                    first_book=rng.randrange(16)
                    words,values,expected_final=stream_for(
                        rng,count,repeat,first_book)
                    if signed:
                        values=[
                            ((-(v&127) if v&128 else v&127)+128)&255
                            for v in values]
                    tables=fixed*16 + bytes(0x400-256)+fixed
                    book=bytearray(0x1C8)
                    for b in range(16):
                        for code in range(16):
                            book[16*b+code]=(code+b)&15
                        struct.pack_into("<I",book,0x100+4*b,4)
                        struct.pack_into("<Q",book,0x140+8*b,LOOKUPS+16*b)
                    struct.pack_into("<I",book,0x1C0,first_book)
                    ctx=bytearray(0x48)
                    struct.pack_into("<QQQ",ctx,0,OUTPUT,OUTPUT,0)
                    ctx[0x18:0x28]=lowmap
                    struct.pack_into("<I",ctx,0x28,4)
                    struct.pack_into("<Q",ctx,0x30,LOW_TABLE)
                    struct.pack_into("<IIQ",ctx,0x38,6,63,OUTPUT)
                    stream=bytearray(0x18)
                    struct.pack_into("<Q",stream,8,WORDS)
                    fill=bytes([0xA5])*OUTPUT_BYTES
                    def setup(writer):
                        writer.write(WORDS,words)
                        writer.write(LOOKUPS,tables)
                        writer.write(OUTPUT,fill)
                    result,buffers,writes,_=emulator.call(
                        img.base+rva,[bytes(ctx),bytes(stream),bytes(book),
                                      WORDS+len(words)],
                        "void",setup=setup)
                    if result is not None:
                        raise AssertionError(
                            f"Original 0x{rva:X} fault count={count} "
                            f"repeat={repeat} trial={trial}: {result}")
                    orig_output=b"".join(writes.get(OUTPUT+i,fill[i:i+1])
                                         for i in range(OUTPUT_BYTES))
                    if count and orig_output[:count]!=bytes(values):
                        raise AssertionError(
                            f"Original adaptive decoded unexpected bytes "
                            f"routine={name} count={count} repeat={repeat}, "
                            f"expected={bytes(values).hex()}, actual={orig_output[:count].hex()}")

                    native_output=(ctypes.c_uint8*OUTPUT_BYTES).from_buffer_copy(fill)
                    native_words=(ctypes.c_uint32*WORD_COUNT)(
                        *struct.unpack("<"+"I"*WORD_COUNT,words))
                    native_table=(ctypes.c_uint8*len(tables)).from_buffer_copy(tables)
                    table_base=ctypes.addressof(native_table)
                    adaptive=Adaptive()
                    for b in range(16):
                        for code in range(16):
                            adaptive.symbol_map[b][code]=(code+b)&15
                        adaptive.lookup_bits[b]=4
                        adaptive.lookup_table[b]=table_base+16*b
                    adaptive.current_symbol=first_book
                    native=Decoder()
                    native.current=ctypes.cast(native_output,ctypes.POINTER(ctypes.c_uint8))
                    native.end=native.current
                    native.symbol_map[:]=list(lowmap)
                    native.lookup_bits=4
                    native.lookup_table=ctypes.cast(
                        table_base+0x400,ctypes.POINTER(ctypes.c_uint8))
                    native.count_bits=6
                    native.capacity=63
                    native.buffer=native.current
                    native_bits=BitStream()
                    native_bits.next_word=ctypes.cast(native_words,ctypes.POINTER(ctypes.c_uint32))
                    last_word=ctypes.cast(ctypes.byref(native_words,len(words)),
                                           ctypes.POINTER(ctypes.c_uint32))
                    fn(ctypes.byref(native),ctypes.byref(native_bits),
                       ctypes.byref(adaptive),last_word)
                    actual=ctypes.string_at(ctypes.addressof(native_output),OUTPUT_BYTES)
                    if actual!=orig_output:
                        at=next(i for i,(a,b) in enumerate(zip(actual,orig_output)) if a!=b)
                        raise AssertionError(f"Adaptive bytes mismatch {name} "
                                             f"count={count} repeat={repeat} byte={at}: "
                                             f"original={orig_output[at]}, source={actual[at]}")
                    orig_current,orig_end=struct.unpack_from("<QQ",buffers[0])
                    base=ctypes.addressof(native_output)
                    got_current=ctypes.cast(native.current,ctypes.c_void_p).value
                    got_end=ctypes.cast(native.end,ctypes.c_void_p).value
                    if (orig_current-OUTPUT,orig_end-OUTPUT)!=(got_current-base,got_end-base):
                        raise AssertionError(f"Adaptive output pointers mismatch {name}")
                    orig_last,=struct.unpack_from("<I",buffers[2],0x1C0)
                    if orig_last!=adaptive.current_symbol:
                        raise AssertionError(f"Adaptive current book mismatch {name}: "
                                             f"original={orig_last}, source={adaptive.current_symbol}")
                    orig_cache,=struct.unpack_from("<I",buffers[1],0)
                    orig_next,=struct.unpack_from("<Q",buffers[1],8)
                    orig_avail,=struct.unpack_from("<I",buffers[1],16)
                    native_next=ctypes.cast(native_bits.next_word,ctypes.c_void_p).value
                    if (orig_cache,(orig_next-WORDS)//4,orig_avail)!=(
                        native_bits.cache,
                        (native_next-ctypes.addressof(native_words))//4,
                        native_bits.available_bits):
                        raise AssertionError(f"Adaptive bitstream mismatch {name} "
                                             f"count={count} repeat={repeat}")
                    cases+=1
        print(f"Original adaptive nibble decoder 0x{rva:X} differential PASS")
    print(f"Bink adaptive Huffman pair differential PASS: {cases} original-vs-source cases")

if __name__=="__main__":main()
