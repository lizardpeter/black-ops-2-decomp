#!/usr/bin/env python3
"""Complete Store MP turret view orchestration differential replay."""
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
        so=pathlib.Path(tmp)/"turret-view.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_turret_view_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.TurretViewMock.argtypes=[C.c_uint];lib.TurretViewMock.restype=C.c_void_p
        lib.TurretViewSize.restype=C.c_uint;lib.TurretViewRun.argtypes=[C.c_void_p,C.c_void_p,C.c_uint,C.c_int,C.c_int,C.c_void_p]
        a=C.create_string_buffer(lib.TurretViewSize());b=C.create_string_buffer(lib.TurretViewSize());rng=random.Random(0x16bfb0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0xa9960,0x39aeb0,0xe6370,0x8a8330,0xc23c0,0x464d00]):
                ptr=lib.TurretViewMock(i)
                if i==3:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for n in range(16384):
                args=(rng.randrange(64),rng.choice([0,100,300,301,1000,2147483647,-2147483648]),rng.choice([0,1,100,300,1000,2147483647,-2147483648]))
                lib.TurretViewRun(base,base,*args,a);lib.TurretViewRun(None,base,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("16384 complete Store turret view comparisons")
if __name__=="__main__":main()
