#!/usr/bin/env python3
"""Original Store orientation and linked roll bodies."""
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
        so=pathlib.Path(tmp)/"angles.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_angles_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.AngleMock.argtypes=[C.c_uint];lib.AngleMock.restype=C.c_void_p
        lib.AngleSize.restype=C.c_uint;lib.AngleRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_void_p]
        a=C.create_string_buffer(lib.AngleSize());b=C.create_string_buffer(lib.AngleSize())
        rng=random.Random(0x462d70)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x461020,0x890da3,0x870bbc,0x464d00,0x8a7dd8]):
                ptr=lib.AngleMock(i if i<4 else 5)
                if i==4:struct.pack_into("<Q",mem,rva,ptr)
                elif i in (1,2):
                    stub=0x1000+0x20*i;mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in range(2):
                if op:mem[0x462d70:0x462d7c]=b"\x48\xb8"+struct.pack("<Q",lib.AngleMock(4))+b"\xff\xe0"
                for n in range(16384):
                    args=(op,rng.randrange(8192),*[bits(rng.choice([-180,-91,-90,-1,-0.0,0,0.1,1,90,180])) for _ in range(3)])
                    lib.AngleRun(base,*args,a);lib.AngleRun(None,*args,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"16384 complete Store orientation comparisons for operation {op}")
if __name__=="__main__":main()
