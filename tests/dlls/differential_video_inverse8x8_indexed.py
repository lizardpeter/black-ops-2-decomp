#!/usr/bin/env python3
"""Original Bink first-frame indexed INTRA quantization, actual x64 vs C++.

Original FUN_18002F2E0 takes a direct quant table pointer. Its original
FUN_1800252C0 caller chooses one of sixteen BINKCONS 8x8 tables at
RVA 0x5B120+256*i. This test verifies all those original tables against
the reconstructed indexed adapter using the original function in Unicorn.
Source files contain copied *original* constants, no decoder substitute.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine  # noqa: E402

SHA="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
RVA=0x2F2E0
QUANT_RVA=0x5B120


def main():
    orig=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/"originals/dlls/binkw64.dll"
    source=Path(sys.argv[2]) if len(sys.argv)>2 else ROOT/"build/bink_intra_indexed.so"
    if hashlib.sha256(orig.read_bytes()).hexdigest()!=SHA:
        raise SystemExit("Pinned Store Bink hash mismatch")
    img=Image(orig)
    m=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    fn=lib.bink_video_inverse8x8_indexed
    fn.argtypes=(ctypes.POINTER(ctypes.c_uint8),ctypes.c_int32,
                 ctypes.POINTER(ctypes.c_int16),ctypes.c_uint32)
    fn.restype=None
    randomizer=random.Random(RVA ^ QUANT_RVA)
    checks=0
    for index in range(16):
        table=bytes(img.data[QUANT_RVA+index*256:QUANT_RVA+(index+1)*256])
        assert len(table)==256
        for sample in range(32):
            if sample==0:coeff=[0]*64
            elif sample==1:coeff=[1024]+[0]*63
            else:coeff=[randomizer.randint(-2048,2047) if randomizer.randrange(5)==0
                        else 0 for _ in range(64)]
            coeff_bytes=struct.pack("<64h",*coeff)
            for stride in (8,16,24):
                start=bytes([0xA5])*(stride*8)
                value,bufs,_,_=m.call(img.base+RVA,
                                     [start,stride,coeff_bytes,table],"void")
                if value is not None:
                    raise AssertionError(f"Original Bink intra fault index={index}: {value}")
                native=(ctypes.c_uint8*len(start)).from_buffer_copy(start)
                cc=(ctypes.c_int16*64).from_buffer_copy(coeff_bytes)
                fn(native,stride,cc,index)
                got=ctypes.string_at(ctypes.addressof(native),len(start))
                if got!=bufs[0]:
                    pos=next(i for i,(a,b) in enumerate(zip(got,bufs[0])) if a!=b)
                    raise AssertionError(f"Original/intra indexed mismatch "
                                         f"q={index},sample={sample},stride={stride},"
                                         f"byte={pos},original={bufs[0][pos]},source={got[pos]}")
                if bufs[1]!=coeff_bytes or bufs[2]!=table:
                    raise AssertionError("Original mutated quant or coefficient input")
                checks+=1
    print(f"Bink original 8x8 intra indexed first-frame quantization PASS: "
          f"{checks} original-vs-source pixel block comparisons over 16 matrices")


if __name__=="__main__":main()
