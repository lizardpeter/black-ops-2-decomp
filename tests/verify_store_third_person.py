#!/usr/bin/env python3
"""Store MP complete third-person camera differential replay."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"third.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_third_person_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ThirdMock.argtypes=[C.c_uint];lib.ThirdMock.restype=C.c_void_p
        lib.ThirdSize.restype=C.c_uint;lib.ThirdRun.argtypes=[C.c_void_p,C.c_void_p,C.c_uint,C.c_uint,C.c_void_p]
        a=C.create_string_buffer(lib.ThirdSize());b=C.create_string_buffer(lib.ThirdSize())
        rng=random.Random(0x16A940)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0xe6370,0xde970,0xa9960,0x39aeb0,0x16a5d0,0x8a7dc0,0x8a7dd8,0x890da3,0xc69f0,0x38bad0,0x870bbc,0x464d00]):
                target=lib.ThirdMock(i)
                if i in (5,6):struct.pack_into("<Q",mem,rva,target)
                elif i in (7,10):
                    stub=0x1000+0x20*i
                    mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",target)+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",target)+b"\xff\xe0"
            for n in range(32768):
                key=rng.randrange(1<<28)
                if n%2:key=(key&~(1|8192|16384))|4|256
                args=(key,rng.randrange(1<<24))
                lib.ThirdRun(base,base,*args,a);lib.ThirdRun(None,base,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete Store third-person camera comparisons")
if __name__=="__main__":main()
