#!/usr/bin/env python3
"""Pinned-original x64 vs reconstructed Bink Huffman table initialization.

Tests original FUN_180023840 at RVA 0x23840, called with four original
arguments: 24-byte bitreader, 8-byte table pointer output, 4-byte table
width output, and 16-byte symbol permutation output.  The C++ implementation
executes independently, with the 976-byte immutable codebook lookup data
recovered from BINKCONS in the SHA256-pinned original DLL.

Test all 16 table types, explicit-prefix permutations, and four hierarchical
shuffle depths. Compare every output byte, original/translated table offsets
and bytes, and complete bitreader state. Input streams use valid unique
symbol sets and enough padding; this is not full video playback.
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
from differential_entropy_bytes import BitStream  # noqa: E402

SHA="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA=0x23840
RVA_LOOKUPS=0x5AD40
RVA_WIDTHS=0x5A740
RVA_OFFSETS=0x5A800
WORDS=OBJECTS+0x1000
WORD_COUNT=64

def bits_for(type_code, rng, mode, arg):
    parts=[]
    def push(value,width):
        parts.extend((value>>b)&1 for b in range(width))
    push(type_code,4)
    if type_code:
        push(mode,1)
        if mode==1:
            num=arg
            push(num-1,3)
            numbers=rng.sample(range(16),num)
            for number in numbers:
                push(number,4)
        else:
            depth=arg
            push(depth,2)
            for _ in range(8):
                push(rng.randrange(2),1)
            # Encode enough bits for every merge even if one half fills
            # early. The decoder must NOT consume unused previewed bits.
            for level in range(1,depth+1):
                width=(1<<(level+1))-1
                blocks=16//(1<<(level+1))
                for _ in range(blocks):
                    push(rng.getrandbits(width),width)
    # Padding protects original lookahead into next word, and deliberately
    # uses nonzero patterns to catch premature word-advancement bugs.
    for _ in range(128):
        push(rng.getrandbits(1),1)
    words=[0]*WORD_COUNT
    for i,bit in enumerate(parts):
        words[i//32] |= bit<<(i%32)
    return struct.pack("<"+"I"*WORD_COUNT,*words)

def main():
    original=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    libpath=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_huffman_codebook.so"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=SHA:
        raise SystemExit("Pinned original Bink DLL SHA-256 mismatch")
    img=Image(original)
    machine=Machine(img)
    lib=ctypes.CDLL(str(libpath.resolve()))
    fn=lib.bink_init_huffman_codebook
    fn.argtypes=(ctypes.POINTER(BitStream),ctypes.POINTER(ctypes.c_void_p),
                 ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8))
    fn.restype=None
    lib.bink_original_huffman_lookup_base.argtypes=()
    lib.bink_original_huffman_lookup_base.restype=ctypes.c_void_p
    native_table_base=lib.bink_original_huffman_lookup_base()
    assert native_table_base

    rng=random.Random(RVA)
    cases=[]
    for t in range(16):
        if t==0:
            cases.extend((t,0,0) for _ in range(16))
        else:
            for n in range(1,9):
                cases.extend((t,1,n) for _ in range(6))
            for depth in range(4):
                cases.extend((t,0,depth) for _ in range(20))

    success=0
    for case,(t,mode,arg) in enumerate(cases):
        words=bits_for(t,rng,mode,arg)
        bitrecord=bytearray(24)
        struct.pack_into("<Q",bitrecord,8,WORDS)
        table_slot=bytes(8)
        width_slot=bytes(4)
        map_slot=bytes([0xA5])*16
        def setup(writer):writer.write(WORDS,words)
        result,bufs,_,_=machine.call(img.base+RVA,
            [bytes(bitrecord),table_slot,width_slot,map_slot],
            "void",setup=setup)
        if result is not None:
            raise AssertionError(f"Original codebook initialization fault case={case}: {result}")
        orig_lookup=struct.unpack_from("<Q",bufs[1])[0]
        orig_width=struct.unpack_from("<I",bufs[2])[0]
        orig_symbols=bufs[3]
        original_offset=orig_lookup-(img.base+RVA_LOOKUPS)
        if orig_width<4 or orig_width>7 or original_offset<0 or original_offset+ (1<<orig_width)>976:
            raise AssertionError(f"Original selected invalid table for type {t}")
        assert len(set(orig_symbols))==16,(
            f"Original result not permutation case={case}: {orig_symbols.hex()}")

        native_words=(ctypes.c_uint32*WORD_COUNT)(
            *struct.unpack("<"+"I"*WORD_COUNT,words))
        native_stream=BitStream()
        native_stream.next_word=ctypes.cast(native_words,ctypes.POINTER(ctypes.c_uint32))
        native_lookup=ctypes.c_void_p()
        native_width=ctypes.c_uint32(0)
        native_symbols=(ctypes.c_uint8*16)(*([0xA5]*16))
        fn(ctypes.byref(native_stream),ctypes.byref(native_lookup),
           ctypes.byref(native_width),native_symbols)
        relative_native_lookup=native_lookup.value-native_table_base
        native_symbol_bytes=bytes(native_symbols)
        if (original_offset,orig_width,orig_symbols)!=(
                relative_native_lookup,native_width.value,native_symbol_bytes):
            raise AssertionError(
                f"Bink codebook result mismatch case={case}, type={t} "
                f"mode={mode} arg={arg}: original="
                f"{(original_offset,orig_width,orig_symbols.hex())}, "
                f"source={(relative_native_lookup,native_width.value,native_symbol_bytes.hex())}")
        original_table=bytes(img.data[RVA_LOOKUPS+original_offset:
            RVA_LOOKUPS+original_offset+(1<<orig_width)])
        native_table=ctypes.string_at(native_lookup.value,1<<orig_width)
        if original_table!=native_table:
            raise AssertionError(f"Embedded table bytes differ type={t}")

        original_cache=struct.unpack_from("<I",bufs[0],0)[0]
        original_next=struct.unpack_from("<Q",bufs[0],8)[0]
        original_available=struct.unpack_from("<I",bufs[0],16)[0]
        native_next=ctypes.cast(native_stream.next_word,ctypes.c_void_p).value
        orig_state=(original_cache,(original_next-WORDS)//4,original_available)
        native_state=(native_stream.cache,
            (native_next-ctypes.addressof(native_words))//4,
            native_stream.available_bits)
        if orig_state!=native_state:
            raise AssertionError(
                f"Bink bitreader mismatch case={case} type={t},mode={mode},arg={arg}: "
                f"original={orig_state}, native={native_state}")
        success+=1
    print(f"Bink original 0x{RVA:X} codebook init PASS: {success} "
          f"table/mapping/cursor cases across all 16 types and four shuffle depths")

if __name__=="__main__":main()
