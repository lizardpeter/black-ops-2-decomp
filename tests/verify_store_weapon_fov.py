#!/usr/bin/env python3
"""Complete original Store MP CG_GetWeaponViewFov differential replay."""
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
        so=pathlib.Path(tmp)/"weapon-fov.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_weapon_fov_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.WeaponFovMock.argtypes=[C.c_uint];lib.WeaponFovMock.restype=C.c_void_p
        lib.WeaponFovTraceSize.restype=C.c_uint
        lib.WeaponFovRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_int,C.c_int,C.c_void_p]
        old=C.create_string_buffer(lib.WeaponFovTraceSize());new=C.create_string_buffer(lib.WeaponFovTraceSize())
        rng=random.Random(0xC12F0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            struct.pack_into("<Q",mem,0x8a7dc0,lib.WeaponFovMock(0))
            for i,rva in enumerate([0x65b8c0,0x65c600,0x646920,0x65b060,0x65b120],1):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.WeaponFovMock(i))+b"\xff\xe0"
            for case in range(32768):
                key=rng.randrange(1<<18)
                if case%2:key=(key|32)&~16384  # ensure sustained ADS path coverage
                now=rng.choice([0,25,100,125,150,1000,2147483647,-2147483648])
                last=rng.choice([0,10,100,1000,2147483647,-2147483648])
                args=(key,rng.randrange(1<<32),
                      bits(rng.choice([0,0.1,0.5,0.75,1,1.5,-0.25,float("nan")])),
                      bits(rng.choice([0,0.1,0.5,1,1.25,-0.5,float("nan")])),
                      bits(rng.choice([0,0.1,0.5,1,2,-0.5,float("nan")])),now,last)
                lib.WeaponFovRun(base,*args,old);lib.WeaponFovRun(None,*args,new)
                assert old.raw==new.raw,(case,args,old.raw.hex(),new.raw.hex())
            print("32768 complete original Store MP CG_GetWeaponViewFov comparisons; variable zoom, timer, transition and result")
if __name__=="__main__":
    main()
