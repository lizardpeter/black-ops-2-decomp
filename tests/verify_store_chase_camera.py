#!/usr/bin/env python3
"""Full Store chase-camera original-body differential comparison."""
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
        so=pathlib.Path(tmp)/"chase.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_chase_camera_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ChaseMock.argtypes=[C.c_uint];lib.ChaseMock.restype=C.c_void_p
        lib.ChaseSize.restype=C.c_uint;lib.ChaseRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_void_p]
        a=C.create_string_buffer(lib.ChaseSize());b=C.create_string_buffer(lib.ChaseSize());rng=random.Random(0x16a5d0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dc0,0x8a7dd8,0x5cf280,0x890da3,0x870bbc,0xc69f0,0x464d00]):
                ptr=lib.ChaseMock(i)
                if i<2:struct.pack_into("<Q",mem,rva,ptr)
                elif i in (3,4):
                    stub=0x1000+0x20*i;mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for n in range(32768):
                args=(rng.randrange(1<<18),rng.randrange(512),*[bits(rng.choice([-720,-360,-180,-90,-45,-1,0,0.1,1,45,90,180,360,720,1234.5])) for _ in range(3)])
                lib.ChaseRun(base,*args,a);lib.ChaseRun(None,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete Store chase-camera comparisons")
if __name__=="__main__":main()
