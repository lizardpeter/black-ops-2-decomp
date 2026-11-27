#!/usr/bin/env python3
"""Compare complete Store MP angle gate, command finalization and frame dispatch."""
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
        so=pathlib.Path(tmp)/"input.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_input_support_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.InputMock.argtypes=[C.c_uint];lib.InputMock.restype=C.c_void_p
        lib.InputSize.restype=C.c_uint;lib.InputRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_void_p]
        a=C.create_string_buffer(lib.InputSize());b=C.create_string_buffer(lib.InputSize());rng=random.Random(0x1930f0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x5f8550,0x8a7dc0,0x8a7960,0x8a78f8,0x8a78e8,0x499a10,0x499460]):
                ptr=lib.InputMock(i)
                if 1<=i<=4:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in range(3):
                for n in range(8192):
                    v=[-1e20,-720,-180,-1,-0.0,0,0.1,90,180,360,720,1e20,float("nan"),float("inf")]
                    args=(op,rng.randrange(1<<20),*[bits(rng.choice(v)) for _ in range(3)])
                    lib.InputRun(base,*args,a);lib.InputRun(None,*args,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 complete Store input support comparisons for operation {op}")
if __name__=="__main__":main()
