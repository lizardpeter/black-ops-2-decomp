#!/usr/bin/env python3
"""Full original camera support function comparisons."""
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
        so=pathlib.Path(tmp)/"support.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_camera_support_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.SupportMock.restype=C.c_void_p
        lib.SupportRun.argtypes=[C.c_void_p,C.c_uint,C.c_void_p]
        lib.SupportAdsRun.argtypes=[C.c_void_p,*([C.c_uint]*6),C.c_void_p]
        a=(C.c_uint64*9)();b=(C.c_uint64*9)()
        rng=random.Random(0xC1240)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            mem[0x18e600:0x18e60c]=b"\x48\xb8"+struct.pack("<Q",lib.SupportMock())+b"\xff\xe0"
            for key in range(4096):
                lib.SupportRun(base,key,a);lib.SupportRun(None,key,b)
                assert list(a)==list(b),(key,list(a),list(b))
            print("8192 original extra-mode and spike-view comparisons (4096 per body)")
            bits=[0,0x80000000,0x3f000000,0x3f800000,0x40000000,0x7fc00000,0x7f800000,0xff800000]
            for case in range(8192):
                args=(case%64,*[rng.choice(bits) for _ in range(5)])
                lib.SupportAdsRun(base,*args,a);lib.SupportAdsRun(None,*args,b)
                assert list(a)==list(b),(case,args,list(a),list(b))
            print("16384 original ADS transition accessor comparisons (8192 per body)")
if __name__=="__main__":main()
