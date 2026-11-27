#!/usr/bin/env python3
"""Compare complete original Store MP HUD/FOV mode helper (140101410)."""
import ctypes as C
import itertools
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT, map_image

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"fov.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_cg_fov_mode_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.FovModeMock.argtypes=[C.c_uint]
        lib.FovModeMock.restype=C.c_void_p
        lib.FovModeSize.restype=C.c_uint
        lib.FovModeRun.argtypes=[C.c_void_p,*([C.c_uint]*4),C.c_void_p]
        lib.FovModeRun.restype=None
        old=C.create_string_buffer(lib.FovModeSize())
        new=C.create_string_buffer(lib.FovModeSize())
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            struct.pack_into("<Q",mem,0x8a7dc0,lib.FovModeMock(0))
            for i,rva in enumerate([0x46cef0,0x46d040],1):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.FovModeMock(i))+b"\xff\xe0"
            count=0
            for args in itertools.product(range(2),range(3),range(128),range(6)):
                lib.FovModeRun(base,*args,old)
                lib.FovModeRun(None,*args,new)
                assert old.raw==new.raw,(args,old.raw.hex(),new.raw.hex())
                count+=1
            print(f"{count} complete original HUD/FOV mode replays; both modes, DualPlay conversions, all null-pointer combinations")
if __name__=="__main__":
    main()
