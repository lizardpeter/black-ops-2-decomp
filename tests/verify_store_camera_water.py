#!/usr/bin/env python3
"""Original Store water-height wrapper, including geometry query layout."""
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
        so=pathlib.Path(tmp)/"water.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_water_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.WaterMock.argtypes=[C.c_uint];lib.WaterMock.restype=C.c_void_p
        lib.WaterSize.restype=C.c_uint;lib.WaterRun.argtypes=[C.c_void_p,*([C.c_uint]*6),C.c_void_p]
        a=C.create_string_buffer(lib.WaterSize());b=C.create_string_buffer(lib.WaterSize());rng=random.Random(0x38bad0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            struct.pack_into("<Q",mem,0x8a7dc0,lib.WaterMock(0))
            for i,rva in enumerate([0x3e2950,0x38b680],1):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.WaterMock(i))+b"\xff\xe0"
            for n in range(16384):
                vals=[-200000,-131072,-1000,-200,0,1,200,1000,200000]
                args=(rng.randrange(128),*[bits(rng.choice(vals)) for _ in range(3)],bits(rng.choice([-1,0,0.1,0.5,1,2,float("nan")])),bits(rng.choice(vals)))
                lib.WaterRun(base,*args,a);lib.WaterRun(None,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("16384 complete Store water-height wrapper comparisons")
if __name__=="__main__":main()
