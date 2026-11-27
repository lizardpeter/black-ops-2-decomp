#!/usr/bin/env python3
"""Complete original Store MP turret FOV comparison."""
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
        so=pathlib.Path(tmp)/"turret.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_turret_fov_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.TurretMock.argtypes=[C.c_uint];lib.TurretMock.restype=C.c_void_p
        lib.TurretTraceSize.restype=C.c_uint
        lib.TurretRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_void_p]
        old=C.create_string_buffer(lib.TurretTraceSize());new=C.create_string_buffer(lib.TurretTraceSize())
        rng=random.Random(0xC1A00)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x18da10,0x65b060,0x65b120]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.TurretMock(i))+b"\xff\xe0"
            for key in range(16384):
                args=(key,bits(rng.choice([0,0.1,0.5,0.75,1,1.5,-0.25,float("nan")])),
                      bits(rng.choice([0,20,55,90,-1,float("nan")])),
                      bits(rng.choice([0,0.1,0.5,1,2,-0.5,float("nan")])),
                      bits(rng.choice([20,40,60,99,float("nan")])))
                lib.TurretRun(base,*args,old);lib.TurretRun(None,*args,new)
                assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
            print("16384 complete Store MP CG_GetTurretViewFov comparisons")
if __name__=="__main__":
    main()
