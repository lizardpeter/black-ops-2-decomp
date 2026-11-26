#!/usr/bin/env python3
"""Complete Store vehicle view offset differential comparison."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image
def bits(f):return struct.unpack("<I",struct.pack("<f",f))[0]
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"vehicle-offset.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_vehicle_offset_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.VehicleOffsetMock.argtypes=[C.c_uint];lib.VehicleOffsetMock.restype=C.c_void_p
        lib.VehicleOffsetSize.restype=C.c_uint;lib.VehicleOffsetRun.argtypes=[C.c_void_p,C.c_void_p,*([C.c_uint]*3),C.c_int,C.c_int,C.c_void_p]
        a=C.create_string_buffer(lib.VehicleOffsetSize());b=C.create_string_buffer(lib.VehicleOffsetSize());rng=random.Random(0xc66d0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x5e07e0,0xa9960,0x39aeb0,0x8a7dc0,0x6468b0,0x18da90,0x18da10,0x6448b0,0x8a8330]):
                ptr=lib.VehicleOffsetMock(i)
                if i in (3,8):struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for n in range(32768):
                vals=[-720,-180,-90,-0.0,0,0.1,30,90,180,360,720]
                args=(rng.randrange(1<<21),bits(rng.choice(vals)),bits(rng.choice(vals)),rng.choice([0,100,1000,2147483647,-2147483648]),rng.choice([0,100,1000,2147483647,-2147483648]))
                lib.VehicleOffsetRun(base,base,*args,a);lib.VehicleOffsetRun(None,base,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete Store vehicle offset comparisons")
if __name__=="__main__":main()
