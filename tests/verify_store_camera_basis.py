#!/usr/bin/env python3
"""Four complete Store camera basis/helper differential replays."""
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
        so=pathlib.Path(tmp)/"basis.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_basis_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.BasisMock.argtypes=[C.c_uint];lib.BasisMock.restype=C.c_void_p
        lib.BasisSize.restype=C.c_uint;lib.BasisRun.argtypes=[C.c_void_p,*([C.c_uint]*6),C.c_void_p]
        a=C.create_string_buffer(lib.BasisSize());b=C.create_string_buffer(lib.BasisSize());rng=random.Random(0x461020)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x870bbc,0x870bc8]):
                stub=0x1000+0x20*i
                mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",lib.BasisMock(i))+b"\xff\xe0"
                mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
            # Verify the actual Store jump table, not reference order alone.
            expected=[0x16bee5,0x16bea7,0x16bf53,0x16bf15,0x16be40,0x16be70]
            assert list(struct.unpack_from("<6I",mem,0x16bf8c))==expected
            for op in range(4):
                for n in range(8192):
                    values=[-100,-3,-1,-0.0,0,0.1,0.5,1,2,100]
                    args=(op,rng.randrange(1<<20),*[bits(rng.choice(values)) for _ in range(3)],
                        rng.choice([1,2,64,256,1024,0x7fffffff,0x80000000]) if op==2 else bits(rng.choice(values)))
                    lib.BasisRun(base,*args,a);lib.BasisRun(None,*args,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((op,n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 complete Store basis function comparisons for operation {op}")
if __name__=="__main__":main()
