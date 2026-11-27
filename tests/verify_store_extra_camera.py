#!/usr/bin/env python3
"""Differential replay for the complete original Store extra-camera body."""
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
        so=pathlib.Path(tmp)/"extra.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_extra_camera_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ExtraMock.argtypes=[C.c_uint];lib.ExtraMock.restype=C.c_void_p
        lib.ExtraTraceSize.restype=C.c_uint
        lib.ExtraRun.argtypes=[C.c_void_p,*([C.c_uint]*4),C.c_void_p];lib.ExtraRun.restype=None
        old=C.create_string_buffer(lib.ExtraTraceSize());new=C.create_string_buffer(lib.ExtraTraceSize())
        rng=random.Random(0x16CE80)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0xc1240,0x173ed0,0x16bda0,0x100fc0,0xc3ae0,0xc3e10,0xc3390,0x464d00,0x892d02]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.ExtraMock(i))+b"\xff\xe0"
            struct.pack_into("<Q",mem,0x8a7dd8,lib.ExtraMock(9))
            for case in range(16384):
                key=rng.randrange(65536)
                args=(key,rng.randrange(65536),bits(rng.choice([0,30,65,90,120,-1,float("nan")])),
                      bits(rng.choice([0,-1,0.5,1,4/3,16/9,32/9,float("nan")])))
                lib.ExtraRun(base,*args,old);lib.ExtraRun(None,*args,new)
                assert old.raw==new.raw,(case,args,old.raw.hex(),new.raw.hex())
            print("16384 complete original Store extra-camera comparisons; 30 output words and ordered calls")
if __name__=="__main__":main()
