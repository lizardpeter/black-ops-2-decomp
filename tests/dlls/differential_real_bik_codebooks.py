#!/usr/bin/env python3
"""Differential 23 consecutive Huffman codebooks from REAL BO2 .bik frames.

FUN_1800252C0 begins each plane by calling FUN_180023840 23 times:
two common, sixteen block tables, then five additional. Feed the actual
first luma bitstream of each SHA-256-pinned BO2 video-only fixture, chaining
both original x64 and reconstructed C++ bitreader state between calls.
Compare the selected table bytes, symbol maps and complete bitreader state.
This verifies real-file codebook parsing, NOT full decoded video frames.
"""
import ctypes
import hashlib
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
sys.path.insert(0,str(ROOT/"tools/dlls"))
from harness import Image,Machine,OBJECTS  # noqa: E402
from differential_entropy_bytes import BitStream  # noqa: E402
from fetch_bik_fixtures import FIXTURES  # noqa: E402

PIN="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
WORDS=OBJECTS+0x1000
LOOKUP_RVA=0x5AD40
STAGES=23

def state(record,base):
    cache,=struct.unpack_from("<I",record,0)
    next_word,=struct.unpack_from("<Q",record,8)
    available,=struct.unpack_from("<I",record,16)
    assert next_word>=base and (next_word-base)%4==0
    return cache,(next_word-base)//4,available

def native_state(stream,words):
    next_word=ctypes.cast(stream.next_word,ctypes.c_void_p).value
    return stream.cache,(next_word-ctypes.addressof(words))//4,stream.available_bits

def first_luma(filedata):
    assert filedata[:4]==b"BIKi"
    frames,=struct.unpack_from("<I",filedata,8)
    assert frames>0
    raw_first,raw_next=struct.unpack_from("<II",filedata,44)
    first,end=raw_first&~1,raw_next&~1
    assert raw_first&1 and 44+4*(frames+1)<=first<end<=len(filedata)
    packet=filedata[first:end]
    split,=struct.unpack_from("<I",packet)
    assert 8<=split<=len(packet) and split%4==0
    return packet[4:split],first,split,len(packet)

def main():
    original=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_huffman_codebook.so"
    fixture_dir=Path(sys.argv[3]) if len(sys.argv)>3 else ROOT/"originals/bik"
    if hashlib.sha256(original.read_bytes()).hexdigest()!=PIN:
        raise SystemExit("Pinned original DLL hash mismatch")
    img=Image(original)
    emulator=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    fn=lib.bink_init_huffman_codebook
    fn.argtypes=(ctypes.POINTER(BitStream),ctypes.POINTER(ctypes.c_void_p),
                 ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint8))
    fn.restype=None
    lib.bink_original_huffman_lookup_base.restype=ctypes.c_void_p
    native_table_base=lib.bink_original_huffman_lookup_base()
    assert native_table_base
    compared=0
    for name,sha in FIXTURES.items():
        data=(fixture_dir/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=sha:
            raise AssertionError(f"Pinned fixture SHA mismatch {name}")
        luma,offset,split,packet_size=first_luma(data)
        raw=luma+bytes(32)   # valid lookahead padding, never a codec library
        words=(ctypes.c_uint32*(len(raw)//4))(
            *struct.unpack("<"+"I"*(len(raw)//4),raw))
        native_bits=BitStream()
        native_bits.next_word=ctypes.cast(words,ctypes.POINTER(ctypes.c_uint32))
        orig_bits=bytearray(24)
        struct.pack_into("<Q",orig_bits,8,WORDS)
        selected=[]
        last=0
        for stage in range(STAGES):
            def setup(w):w.write(WORDS,raw)
            result,buffers,_,_=emulator.call(
                img.base+0x23840,
                [bytes(orig_bits),bytes(8),bytes(4),bytes([0xA5])*16],
                "void",setup=setup)
            if result is not None:
                raise AssertionError(f"Original codebook fault {name} stage={stage}: {result}")
            orig_bits=bytearray(buffers[0])
            orig_ptr,=struct.unpack_from("<Q",buffers[1])
            orig_width,=struct.unpack_from("<I",buffers[2])
            orig_map=buffers[3]
            orig_offset=orig_ptr-img.base-LOOKUP_RVA
            assert 4<=orig_width<=7 and 0<=orig_offset and orig_offset+(1<<orig_width)<=976
            original_table=img.data[LOOKUP_RVA+orig_offset:LOOKUP_RVA+orig_offset+(1<<orig_width)]
            native_ptr=ctypes.c_void_p()
            native_width=ctypes.c_uint32()
            native_map=(ctypes.c_uint8*16)()
            fn(ctypes.byref(native_bits),ctypes.byref(native_ptr),
               ctypes.byref(native_width),native_map)
            noff=native_ptr.value-native_table_base
            if (orig_offset,orig_width,orig_map)!=(noff,native_width.value,bytes(native_map)):
                raise AssertionError(f"Real codebook mismatch {name} stage={stage} "
                                     f"original={(orig_offset,orig_width,orig_map.hex())}, "
                                     f"source={(noff,native_width.value,bytes(native_map).hex())}")
            assert ctypes.string_at(native_ptr.value,1<<orig_width)==original_table
            ob=state(orig_bits,WORDS)
            nb=native_state(native_bits,words)
            if ob!=nb:
                raise AssertionError(f"Real bitstream mismatch {name} stage={stage}: "
                                     f"original={ob} source={nb}")
            consumed=32*ob[1]-ob[2]
            if consumed<=last:
                raise AssertionError(f"Real codebook stream did not advance {name} {stage}")
            last=consumed
            selected.append(orig_width)
            compared+=1
        assert last<8*len(luma)
        print(f"{name}: original first keyframe {offset}, {packet_size} bytes, "
              f"luma split={split}, 23 REAL codebook mappings and cursors PASS, "
              f"consumed {last} bits, widths={selected}")
    print(f"Real BO2 video first-keyframe Huffman init differential PASS: "
          f"{compared} chained codebooks across {len(FIXTURES)} SHA-pinned media files")

if __name__=="__main__":main()
