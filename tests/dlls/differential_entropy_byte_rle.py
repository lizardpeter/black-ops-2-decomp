#!/usr/bin/env python3
"""Original pinned Bink 0x24140 vs reconstructed Huffman byte/RLE decoder.

The Store DLL's exact x64 instructions execute under Unicorn. The independent
reconstructed C++ executes natively. We compare output to independently
generated tokens, all decoded bytes, output-pointer state and the complete
LSB-first input-reservoir state. Fixed 4-bit and variable 1/2/3-bit Huffman
codes, all 4/8/12/32 RLE lengths, zero-count and repeated-literal cases are
covered. No external compression or decoder library is used.
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
from differential_entropy_bytes import (  # noqa: E402
    Decoder, BitStream, FIXED_TABLE, VARIABLE_TABLE, VARIABLE_CODES,
)

PIN = "10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA = 0x24140
WORDS, LOOKUP, OUTPUT = OBJECTS+0x1000, OBJECTS+0x2000, OBJECTS+0x3000
OUTPUT_BYTES, WORD_COUNT = 128, 64
RUN_LENGTHS = {12: 4, 13: 8, 14: 12, 15: 32}


def create(rng, count, repeated, variable):
    bits = []
    def add(value, width):
        bits.extend((value >> i) & 1 for i in range(width))

    add(count, 6)
    values = []
    symbols = list(range(16))
    if variable:
        symbols[:4] = [3, 12, 5, 13]
    if count:
        add(int(repeated), 1)
        if repeated:
            literal = rng.randrange(16)
            add(literal, 4)
            values = [literal] * count
        else:
            previous = 0
            while len(values) < count:
                remaining = count - len(values)
                code_pool = range(4) if variable else range(16)
                valid = [c for c in code_pool
                         if symbols[c] < 12 or RUN_LENGTHS[symbols[c]] <= remaining]
                code = rng.choice(valid)
                symbol = symbols[code]
                code_bits, width = (VARIABLE_CODES[code] if variable
                                    else (code, 4))
                add(code_bits, width)
                if symbol < 12:
                    previous = symbol
                    values.append(previous)
                else:
                    values.extend([previous] * RUN_LENGTHS[symbol])

    words = [0] * WORD_COUNT
    for i, value in enumerate(bits):
        words[i // 32] |= value << (i % 32)
    return struct.pack("<" + "I" * WORD_COUNT, *words), bytes(values), bytes(symbols)


def main():
    original = Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source = Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_entropy_byte_rle.so"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=PIN:
        raise SystemExit("Original Bink SHA-256 mismatch")
    image=Image(original)
    machine=Machine(image)
    lib=ctypes.CDLL(str(source.resolve()))
    function=lib.bink_decode_entropy_byte_rle
    function.argtypes=(ctypes.POINTER(Decoder),ctypes.POINTER(BitStream),
                       ctypes.POINTER(ctypes.c_uint32))
    function.restype=None
    rng=random.Random(RVA)
    cases=[]
    for count in (0,1,2,3,4,7,8,9,12,15,16,31,32,47,63):
        for repeated in ((False,True) if count else (False,)):
            for variable in ((False,True) if count and not repeated else (False,)):
                for n in range(12):
                    cases.append((count,repeated,variable,n))
    checked=0
    for case,(count,repeated,variable,_) in enumerate(cases):
        words,expected,symbols=create(rng,count,repeated,variable)
        table=VARIABLE_TABLE if variable else FIXED_TABLE
        output_initial=bytes([0xA5])*OUTPUT_BYTES

        decoder=bytearray(0x48)
        struct.pack_into("<QQQ",decoder,0,OUTPUT,OUTPUT,0)
        decoder[0x18:0x28]=symbols
        struct.pack_into("<I",decoder,0x28,4)
        struct.pack_into("<Q",decoder,0x30,LOOKUP)
        struct.pack_into("<IIQ",decoder,0x38,6,63,OUTPUT)
        bitreader=bytearray(24)
        struct.pack_into("<Q",bitreader,8,WORDS)
        def setup(writer):
            writer.write(WORDS,words)
            writer.write(LOOKUP,table)
            writer.write(OUTPUT,output_initial)

        original_result,buffers,writes,_=machine.call(
            image.base+RVA,
            [bytes(decoder),bytes(bitreader),WORDS+len(words)],
            "void",setup=setup)
        if original_result is not None:
            raise AssertionError(f"Original 0x{RVA:X} fault case {case}: {original_result}")
        original_output=b"".join(
            writes.get(OUTPUT+i,output_initial[i:i+1]) for i in range(OUTPUT_BYTES))
        if count and original_output[:count]!=expected:
            raise AssertionError(f"Original RLE output disagrees with encoded expected "
                                 f"case={case}: want={expected.hex()} "
                                 f"got={original_output[:count].hex()}")

        native_output=(ctypes.c_uint8*OUTPUT_BYTES).from_buffer_copy(output_initial)
        native_table=(ctypes.c_uint8*16).from_buffer_copy(table)
        native_words=(ctypes.c_uint32*WORD_COUNT)(*struct.unpack("<"+"I"*WORD_COUNT,words))
        native=Decoder()
        native.current=ctypes.cast(native_output,ctypes.POINTER(ctypes.c_uint8))
        native.end=ctypes.cast(native_output,ctypes.POINTER(ctypes.c_uint8))
        native.symbol_map[:]=list(symbols)
        native.lookup_bits=4
        native.lookup_table=ctypes.cast(native_table,ctypes.POINTER(ctypes.c_uint8))
        native.count_bits=6
        native.capacity=63
        native.buffer=ctypes.cast(native_output,ctypes.POINTER(ctypes.c_uint8))
        stream=BitStream()
        stream.next_word=ctypes.cast(native_words,ctypes.POINTER(ctypes.c_uint32))
        last=ctypes.cast(ctypes.byref(native_words,len(words)),
                         ctypes.POINTER(ctypes.c_uint32))
        function(ctypes.byref(native),ctypes.byref(stream),last)

        actual=ctypes.string_at(ctypes.addressof(native_output),OUTPUT_BYTES)
        if actual!=original_output:
            index=next(i for i,(a,b) in enumerate(zip(actual,original_output)) if a!=b)
            raise AssertionError(f"RLE original vs source mismatch case={case} "
                                 f"count={count} variable={variable} repeated={repeated} "
                                 f"byte={index}: original={original_output[index]} source={actual[index]}")

        oc,oe=struct.unpack_from("<QQ",buffers[0])
        native_base=ctypes.addressof(native_output)
        nc=ctypes.cast(native.current,ctypes.c_void_p).value
        ne=ctypes.cast(native.end,ctypes.c_void_p).value
        if (oc-OUTPUT,oe-OUTPUT)!=(nc-native_base,ne-native_base):
            raise AssertionError(f"RLE decoder pointer mismatch case={case}")
        orig_cache,=struct.unpack_from("<I",buffers[1],0)
        orig_next,=struct.unpack_from("<Q",buffers[1],8)
        orig_available,=struct.unpack_from("<I",buffers[1],16)
        native_next=ctypes.cast(stream.next_word,ctypes.c_void_p).value
        if (orig_cache,(orig_next-WORDS)//4,orig_available)!=(
            stream.cache,(native_next-ctypes.addressof(native_words))//4,
            stream.available_bits
        ):
            raise AssertionError(f"RLE decoder bitstream mismatch case={case}")
        checked+=1
    print(f"Bink 0x{RVA:X} Huffman byte/RLE decoder differential PASS: "
          f"{checked} synthetic streams")


if __name__=="__main__":
    main()
