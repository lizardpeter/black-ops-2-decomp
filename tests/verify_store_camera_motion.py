#!/usr/bin/env python3
"""Compare four complete camera-motion bodies using original Store math boundaries."""
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
        so=pathlib.Path(tmp)/"motion.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_motion_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.MotionMock.argtypes=[C.c_uint];lib.MotionMock.restype=C.c_void_p
        lib.MotionSize.restype=C.c_uint
        lib.MotionRun.argtypes=[C.c_void_p,C.c_void_p,*([C.c_uint]*5),C.c_int,C.c_int,C.c_uint,C.c_void_p]
        old=C.create_string_buffer(lib.MotionSize());new=C.create_string_buffer(lib.MotionSize())
        rng=random.Random(0x16C380)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            # CRT adjacent entry points are only six bytes apart; redirect
            # through nearby unused test trampolines rather than overwriting
            # one original entry with the other.
            for i,rva in enumerate([0x890da3,0x890d60,0x870bc2,0x461740,0xa9960,0x39aeb0,0x6f3ba0]):
                if i<3:
                    stub=0x1000+0x20*i
                    mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",lib.MotionMock(i))+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.MotionMock(i))+b"\xff\xe0"
            for op in range(4):
                for seed in range(4096):
                    values=[0,-0.0,0.1,-0.1,30,-90,180,360,-720,1080]
                    if op==2:values=[0,-0.0,0.1,-0.1,1,-2,30,float("nan")]
                    args=(op,rng.randrange(1024),*[bits(rng.choice(values)) for _ in range(3)],
                          rng.choice([0,10,100,500,1000,2147483647,-2147483648]),
                          rng.choice([0,100,1000,2147483647,-2147483648]),bits(rng.choice([0,0.1,0.5,1,5,-1])))
                    lib.MotionRun(base,base,*args,old);lib.MotionRun(None,base,*args,new)
                    if old.raw!=new.raw:
                        x=struct.unpack("<"+str(len(old.raw)//8)+"Q",old.raw)
                        y=struct.unpack("<"+str(len(new.raw)//8)+"Q",new.raw)
                        raise AssertionError((seed,args,[(i,hex(a),hex(b)) for i,(a,b) in enumerate(zip(x,y)) if a!=b][:30]))
                print(f"4096 original camera motion comparisons for operation {op}")
if __name__=="__main__":main()
