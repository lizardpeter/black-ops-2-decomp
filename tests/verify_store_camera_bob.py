#!/usr/bin/env python3
"""Compare original Store horizontal/vertical bob, including all branches."""
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
        so=pathlib.Path(tmp)/"bob.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_bob_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.BobMock.argtypes=[C.c_uint];lib.BobMock.restype=C.c_void_p
        lib.BobSize.restype=C.c_uint;lib.BobRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_void_p]
        a=C.create_string_buffer(lib.BobSize());b=C.create_string_buffer(lib.BobSize())
        rng=random.Random(0x6602a0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            struct.pack_into("<Q",mem,0x8a7dd8,lib.BobMock(0))
            stub=0x1000;mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",lib.BobMock(1))+b"\xff\xe0"
            mem[0x870bc8:0x870bcd]=b"\xe9"+struct.pack("<i",stub-0x870bc8-5)
            for op in range(2):
                for n in range(16384):
                    args=(op,rng.randrange(1<<22),bits(rng.choice([0,0.1,1,3.14,6.28,-1,100])),bits(rng.choice([0,0.1,1,10,100,-1])),bits(rng.choice([0,0.1,0.5,1,2,-1])))
                    lib.BobRun(base,*args,a);lib.BobRun(None,*args,b)
                    assert a.raw==b.raw,(n,args,a.raw.hex(),b.raw.hex())
                print(f"16384 complete original camera bob comparisons for operation {op}")
if __name__=="__main__":main()
