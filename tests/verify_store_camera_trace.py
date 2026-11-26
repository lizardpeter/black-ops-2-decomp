#!/usr/bin/env python3
"""Full original third-person trace wrapper, with controlled collision results."""
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
        so=pathlib.Path(tmp)/"trace.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_trace_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.TraceMock.restype=C.c_void_p;lib.TraceSize.restype=C.c_uint
        lib.TraceRun.argtypes=[C.c_void_p,*([C.c_uint]*3),C.c_void_p]
        a=C.create_string_buffer(lib.TraceSize());b=C.create_string_buffer(lib.TraceSize())
        rng=random.Random(0xC69F0)
        values=[0,0.0001,0.001,0.002,0.1,0.5,0.9,1,1.5,-1]
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            mem[0x190530:0x19053c]=b"\x48\xb8"+struct.pack("<Q",lib.TraceMock())+b"\xff\xe0"
            for n in range(32768):
                args=(rng.randrange(65536),bits(rng.choice(values)),bits(rng.choice(values)))
                lib.TraceRun(base,*args,a);lib.TraceRun(None,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete Store third-person trace wrapper comparisons")
if __name__=="__main__":main()
