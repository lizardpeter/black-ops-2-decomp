#!/usr/bin/env python3
"""Original Store default-view FOV and projection full-body comparisons."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image

def bits(v): return struct.unpack("<I",struct.pack("<f",v))[0]

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"camera-pipeline.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_pipeline_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CameraPipelineMock.argtypes=[C.c_uint];lib.CameraPipelineMock.restype=C.c_void_p
        lib.CameraPipelineSize.restype=C.c_uint
        lib.DefaultFovRun.argtypes=[C.c_void_p,*([C.c_uint]*4),C.c_void_p]
        lib.ProjectionRun.argtypes=[C.c_void_p,C.c_void_p,*([C.c_uint]*3),C.c_int,C.c_int,C.c_uint,C.c_uint,C.c_void_p]
        old=C.create_string_buffer(lib.CameraPipelineSize());new=C.create_string_buffer(lib.CameraPipelineSize())
        rng=random.Random(0xF0A)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in [(0,0x8a7dd8),(1,0x8a7dc0)]:
                struct.pack_into("<Q",mem,rva,lib.CameraPipelineMock(i))
            for i,rva in [(2,0x46cef0),(3,0xc1b90),(4,0x892d02),(5,0x46b300)]:
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.CameraPipelineMock(i))+b"\xff\xe0"
            for op in range(4):
                for seed in range(2048):
                    args=(op,rng.randrange(1<<24) if seed%2 else rng.choice([0,1,256,65536,65793]),rng.randrange(64),rng.randrange(1<<18))
                    lib.DefaultFovRun(base,*args,old);lib.DefaultFovRun(None,*args,new)
                    assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
            print("8192 original default-FOV comparisons across four complete functions")
            for seed in range(8192):
                args=(bits(rng.choice([0,-1,30,65,90,160,float("nan")])),
                      bits(rng.choice([0,-1,0.5,0.999,1,4/3,16/9,21/9,32/9,float("nan")])),
                      rng.randrange(1<<18),rng.choice([0,10,100,500,1000,2000,2147483647,-2147483648]),
                      rng.choice([0,100,1000,2147483647,-2147483648]),bits(rng.choice([0,0.1,1,5,-1,float("nan")])),
                      bits(rng.choice([30,65,120])))
                lib.ProjectionRun(base,base,*args,old);lib.ProjectionRun(None,base,*args,new)
                assert old.raw==new.raw,(seed,args,old.raw.hex(),new.raw.hex())
            print("8192 complete Store CG_CalcFov comparisons; calls, timer, projection and sensitivity fields")
if __name__=="__main__":
    main()
