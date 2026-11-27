#!/usr/bin/env python3
"""Original Store CG_UpdateCameraMode full body comparison."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"mode.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_mode_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ModeMock.argtypes=[C.c_uint];lib.ModeMock.restype=C.c_void_p
        lib.ModeSize.restype=C.c_uint;lib.ModeRun.argtypes=[C.c_void_p,C.c_void_p,C.c_uint,C.c_uint,C.c_void_p]
        a=C.create_string_buffer(lib.ModeSize());b=C.create_string_buffer(lib.ModeSize())
        rng=random.Random(0xC0E20)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0xa9960,0x39aeb0,0x18e600,0xbfcc0,0x56d110,0xc07d0]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.ModeMock(i))+b"\xff\xe0"
            for case in range(32768):
                args=(rng.randrange(1<<24),rng.randrange(65536))
                lib.ModeRun(base,base,*args,a);lib.ModeRun(None,base,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+str(len(a.raw)//8)+"Q",a.raw);y=struct.unpack("<"+str(len(b.raw)//8)+"Q",b.raw)
                    raise AssertionError((case,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete original Store CG_UpdateCameraMode comparisons")
if __name__=="__main__":main()
