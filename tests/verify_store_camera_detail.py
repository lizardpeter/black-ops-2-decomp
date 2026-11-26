#!/usr/bin/env python3
"""Compare three complete original camera detail bodies."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image

def bits(v):return struct.unpack("<I",struct.pack("<f",v))[0]
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"detail.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_detail_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.DetailMock.argtypes=[C.c_uint];lib.DetailMock.restype=C.c_void_p
        lib.DetailSize.restype=C.c_uint;lib.DetailRun.argtypes=[C.c_void_p,C.c_void_p,*([C.c_uint]*5),C.c_int,C.c_int,C.c_void_p]
        a=C.create_string_buffer(lib.DetailSize());b=C.create_string_buffer(lib.DetailSize())
        rng=random.Random(0xC72B0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x5e07e0,0x890da3,0xa9960,0x39aeb0,0xe6370,0xde970,0xc33f0,0xc3650]):
                target=lib.DetailMock(i)
                if i==1:
                    stub=0x1000
                    mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",target)+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",target)+b"\xff\xe0"
            for op in range(3):
                for n in range(8192):
                    args=(op,rng.randrange(1024),*[bits(rng.choice([-1,0,0.1,1,2,5,20,50,1000])) for _ in range(3)],rng.choice([0,1,100,1000,2147483647,-2147483648]),rng.choice([0,1,100,1000,2147483647,-2147483648]))
                    lib.DetailRun(base,base,*args,a);lib.DetailRun(None,base,*args,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((op,n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 complete Store camera detail comparisons for operation {op}")
if __name__=="__main__":main()
