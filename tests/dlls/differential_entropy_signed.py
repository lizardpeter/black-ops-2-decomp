#!/usr/bin/env python3
"""Pinned-original Bink 0x24DB0 vs native reconstructed signed-byte entropy.

Tests repeated signed nibbles, variable- and fixed-length Huffman symbols,
sign-bit consumption, output buffers, cursor/limit pointers, and the complete
LSB-first 32-bit bitreader state on valid synthetically packed bitstreams.
This does NOT constitute full Bink video decoding.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/differential"))
from harness import Image, Machine, OBJECTS  # noqa: E402
from differential_entropy_bytes import (
    Decoder, BitStream, FIXED_TABLE, VARIABLE_TABLE, VARIABLE_CODES,
)

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x24DB0
WORDS, LOOKUP, OUTPUT = OBJECTS+0x1000, OBJECTS+0x2000, OBJECTS+0x3000
LENGTH = 128


def packed_stream(count, repeat, codes, signs, variable):
    bits = []
    def add(value, width):
        bits.extend((value >> i) & 1 for i in range(width))
    add(count, 6)
    if count:
        add(int(repeat), 1)
        if repeat:
            magnitude = codes[0]
            add(magnitude, 4)
            if magnitude:
                add(signs[0], 1)
        else:
            for code, sign in zip(codes, signs):
                value, width = VARIABLE_CODES[code] if variable else (code, 4)
                add(value, width)
                if code:
                    add(sign, 1)
    words = [0]*32
    for i, bit in enumerate(bits):
        words[i//32] |= bit << (i%32)
    return words


def main():
    orig = Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source = Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_entropy_signed.so"
    if hashlib.sha256(orig.read_bytes()).hexdigest() != PIN:
        raise SystemExit("Original Bink hash mismatch")
    image = Image(orig)
    machine = Machine(image)
    lib = ctypes.CDLL(str(source.resolve()))
    func = lib.bink_decode_entropy_signed_bytes
    func.argtypes = (ctypes.POINTER(Decoder),ctypes.POINTER(BitStream),
                     ctypes.POINTER(ctypes.c_uint32))
    func.restype = None

    rng = random.Random(RVA)
    cases = []
    for count in (0,1,2,3,7,8,9,15,16,31,32,47,63):
        for repeat in ((False,True) if count else (False,)):
            for variable in ((False,True) if (count and not repeat) else (False,)):
                for _ in range(12):
                    code_length = 1 if repeat else count
                    codes=[rng.randrange(16 if (repeat or not variable) else 4)
                           for _ in range(code_length)]
                    signs=[rng.randrange(2) for _ in range(code_length)]
                    cases.append((count,repeat,codes,signs,variable))

    symbols=bytes(range(16))
    initial=bytes([0xA5])*LENGTH
    checked=0
    for i,(count,repeat,codes,signs,variable) in enumerate(cases):
        words=packed_stream(count,repeat,codes,signs,variable)
        word_bytes=struct.pack("<32I",*words)
        table=VARIABLE_TABLE if variable else FIXED_TABLE
        state=bytearray(0x48)
        struct.pack_into("<QQQ",state,0,OUTPUT,OUTPUT,0)
        state[0x18:0x28]=symbols
        struct.pack_into("<I",state,0x28,4)
        struct.pack_into("<Q",state,0x30,LOOKUP)
        struct.pack_into("<IIQ",state,0x38,6,63,OUTPUT)
        bitstate=bytearray(24)
        struct.pack_into("<Q",bitstate,8,WORDS)
        def setup(w):
            w.write(WORDS,word_bytes)
            w.write(LOOKUP,table)
            w.write(OUTPUT,initial)
        ret,buffers,writes,_=machine.call(
            image.base+RVA,[bytes(state),bytes(bitstate),WORDS+len(word_bytes)],
            "void",setup=setup)
        if ret is not None:
            raise AssertionError(f"Original 0x{RVA:X} fault case={i}: {ret}")
        oracle=b"".join(writes.get(OUTPUT+n,initial[n:n+1]) for n in range(LENGTH))

        output=(ctypes.c_uint8*LENGTH).from_buffer_copy(initial)
        lookup=(ctypes.c_uint8*16).from_buffer_copy(table)
        stream_words=(ctypes.c_uint32*len(words))(*words)
        d=Decoder()
        d.current=ctypes.cast(output,ctypes.POINTER(ctypes.c_uint8))
        d.end=ctypes.cast(output,ctypes.POINTER(ctypes.c_uint8))
        d.symbol_map[:]=list(symbols)
        d.lookup_bits=4
        d.lookup_table=ctypes.cast(lookup,ctypes.POINTER(ctypes.c_uint8))
        d.count_bits=6
        d.capacity=63
        d.buffer=ctypes.cast(output,ctypes.POINTER(ctypes.c_uint8))
        b=BitStream()
        b.next_word=ctypes.cast(stream_words,ctypes.POINTER(ctypes.c_uint32))
        end=ctypes.cast(ctypes.byref(stream_words,len(word_bytes)),
                        ctypes.POINTER(ctypes.c_uint32))
        func(ctypes.byref(d),ctypes.byref(b),end)
        actual=ctypes.string_at(ctypes.addressof(output),LENGTH)
        if actual!=oracle:
            at=next(k for k,(x,y) in enumerate(zip(actual,oracle)) if x!=y)
            raise AssertionError(f"Signed Bink mismatch case={i} count={count} "
                                 f"repeat={repeat} variable={variable} offset={at}: "
                                 f"original={oracle[at]} source={actual[at]}")
        oracle_head,oracle_tail=struct.unpack_from("<QQ",buffers[0])
        base=ctypes.addressof(output)
        source_head=ctypes.cast(d.current,ctypes.c_void_p).value
        source_tail=ctypes.cast(d.end,ctypes.c_void_p).value
        if (oracle_head-OUTPUT,oracle_tail-OUTPUT)!=(source_head-base,source_tail-base):
            raise AssertionError(f"Signed decoder pointer mismatch case={i}")
        original_cache,=struct.unpack_from("<I",buffers[1],0)
        original_next,=struct.unpack_from("<Q",buffers[1],8)
        original_bits,=struct.unpack_from("<I",buffers[1],16)
        source_next=ctypes.cast(b.next_word,ctypes.c_void_p).value
        if (original_cache,(original_next-WORDS)//4,original_bits)!=(
            b.cache,(source_next-ctypes.addressof(stream_words))//4,b.available_bits
        ):
            raise AssertionError(f"Signed decoder bit reservoir mismatch case={i}")
        checked+=1
    print(f"Bink 0x{RVA:X} signed-byte entropy differential PASS: "
          f"{checked} mixed Huffman/repeat streams")


if __name__=="__main__":
    main()
