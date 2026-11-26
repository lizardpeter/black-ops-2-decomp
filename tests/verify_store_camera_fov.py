#!/usr/bin/env python3
"""Complete Store MP FOV body differential replay with controlled external calls."""
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
        so=pathlib.Path(tmp)/"camera.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_fov_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CameraMock.argtypes=[C.c_uint];lib.CameraMock.restype=C.c_void_p
        lib.CameraTraceSize.restype=C.c_uint
        lib.CameraRun.argtypes=[C.c_void_p,C.c_void_p,C.c_uint,C.c_uint,C.c_uint,C.c_void_p]
        lib.CameraRun.restype=None
        old=C.create_string_buffer(lib.CameraTraceSize());new=C.create_string_buffer(lib.CameraTraceSize())
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x6468b0,0x18da90,0x18da10,0x6448b0,0x18e600,0xc12f0,0xc1a00,0xa9960,0x39aeb0]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.CameraMock(i))+b"\xff\xe0"
            for i,rva in [(9,0x8a7dd8),(10,0x8a7dc0)]:
                struct.pack_into("<Q",mem,rva,lib.CameraMock(i))
            rng=random.Random(0xC1B90)
            configs=[sum(1<<(8+i*3) for i in range(6)),sum(2<<(8+i*3) for i in range(6))]
            for seed in range(8192):
                key=rng.randrange(65536)&~64
                flags=rng.randrange(32768)
                config=(configs[seed%2]|rng.randrange(256)) if seed%4<2 else rng.randrange(1<<26)
                lib.CameraRun(base,base,key,flags,config,old)
                lib.CameraRun(None,base,key,flags,config,new)
                # NaN payload preservation is tested by comparing exact result bits.
                assert old.raw==new.raw,(seed,key,flags,config,old.raw.hex(),new.raw.hex())
            print("8192 complete Store MP FOV body comparisons; original calls and result bits, client 0")
if __name__=="__main__":
    main()
