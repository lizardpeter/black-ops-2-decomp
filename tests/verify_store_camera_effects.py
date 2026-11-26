#!/usr/bin/env python3
"""Compare full Store first-person camera and shake functions."""
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
        so=pathlib.Path(tmp)/"effects.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_effects_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.EffectsMock.argtypes=[C.c_uint];lib.EffectsMock.restype=C.c_void_p
        lib.EffectsSize.restype=C.c_uint
        lib.EffectsRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_int,C.c_uint,C.c_void_p]
        a=C.create_string_buffer(lib.EffectsSize());b=C.create_string_buffer(lib.EffectsSize())
        rng=random.Random(0x16BA40)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x660b40,0x6602a0,0x660470,0x890da3,0x890d60,0x472fe0,0xc72b0,0x8a8330,0x870bc8]):
                addr=lib.EffectsMock(i)
                if i==7:struct.pack_into("<Q",mem,rva,addr)
                elif i in (3,4,8):
                    stub=0x1000+0x20*i
                    mem[stub:stub+12]=b"\x48\xb8"+struct.pack("<Q",addr)+b"\xff\xe0"
                    mem[rva:rva+5]=b"\xe9"+struct.pack("<i",stub-rva-5)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",addr)+b"\xff\xe0"
            for op in range(2):
                for case in range(8192):
                    key=rng.randrange(1<<20)
                    if op==0 and case%2:key &= ~3
                    args=(op,key,rng.choice([-1,0,1,149,150,151,449,450,451,1000]),bits(rng.choice([0,-1,0.25,1,2,float("nan")])))
                    lib.EffectsRun(base,*args,a);lib.EffectsRun(None,*args,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+str(len(a.raw)//8)+"Q",a.raw);y=struct.unpack("<"+str(len(b.raw)//8)+"Q",b.raw)
                        raise AssertionError((case,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 complete camera effects comparisons for operation {op}")
if __name__=="__main__":main()
