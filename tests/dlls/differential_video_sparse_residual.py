#!/usr/bin/env python3
"""Original Bink sparse residual 0x30620 / prediction 0x30A60 differential.

The original x64 and independently compiled C++ are run on the same padded
LSB-first streams with all eight bitplane widths, varied change budgets,
positive/negative coefficients and arbitrary prior pixels. Compare output
and complete relocated original bitreader state, including untouched bytes.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,OBJECTS
from differential_entropy_bytes import BitStream
PIN="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
WORDS=OBJECTS+0x1000
def main():
    orig,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(orig.read_bytes()).hexdigest()==PIN
    img=Image(orig)
    m=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    byteptr=ctypes.POINTER(ctypes.c_uint8)
    bitptr=ctypes.POINTER(BitStream)
    sparse=lib.bink_video_sparse_residual
    sparse.argtypes=(byteptr,bitptr,ctypes.c_uint32)
    sparse.restype=None
    predict=lib.bink_video_residual_prediction
    predict.argtypes=(byteptr,ctypes.c_uint32,bitptr,ctypes.c_uint32,byteptr,ctypes.c_uint32)
    predict.restype=None
    rng=random.Random(0x30620)
    checks=0
    for width in range(8):
        for budget in (0,1,2,7,15,31,63,127):
            for trial in range(4):
                words=[rng.getrandbits(32) for _ in range(128)]
                words[0]=(words[0]&~7)|width
                packed=struct.pack("<128I",*words)
                stream=bytearray(24)
                struct.pack_into("<Q",stream,8,WORDS)
                def setup(w):w.write(WORDS,packed)
                for full in (False,True):
                    if full:
                        stride=(8,16,24,29)[trial]
                        initial=bytes([0xA5])*(stride*8)
                        ref=rng.randbytes(stride*8)
                        result,buffers,_,_=m.call(
                            img.base+0x30A60,[initial,stride,bytes(stream),budget,ref,stride],
                            "void",setup=setup)
                    else:
                        initial=bytes(64)
                        result,buffers,_,_=m.call(img.base+0x30620,
                            [initial,bytes(stream),budget],"void",setup=setup)
                    assert result is None,(width,budget,trial,full,result)
                    dest=(ctypes.c_uint8*len(initial)).from_buffer_copy(initial)
                    native_words=(ctypes.c_uint32*128)(*words)
                    native_bits=BitStream()
                    native_bits.next_word=ctypes.cast(native_words,ctypes.POINTER(ctypes.c_uint32))
                    if full:
                        reference=(ctypes.c_uint8*len(ref)).from_buffer_copy(ref)
                        predict(dest,stride,ctypes.byref(native_bits),budget,reference,stride)
                        assert bytes(reference)==ref and buffers[2]==ref
                    else:
                        sparse(dest,ctypes.byref(native_bits),budget)
                    expected=buffers[0]
                    actual=bytes(dest)
                    if actual!=expected:
                        i=next(i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b)
                        raise AssertionError(f"Residual output mismatch width={width} budget={budget} "
                            f"trial={trial} full={full} byte={i} original={expected[i]} source={actual[i]}")
                    state=bytearray(buffers[1])
                    ptr,=struct.unpack_from("<Q",state,8)
                    struct.pack_into("<Q",state,8,ctypes.addressof(native_words)+ptr-WORDS)
                    assert bytes(state)==ctypes.string_at(ctypes.addressof(native_bits),24),(
                        width,budget,trial,full,state.hex())
                    checks+=1
    print(f"Bink original sparse residual + prediction differential PASS: {checks} cases")
if __name__=="__main__":main()
