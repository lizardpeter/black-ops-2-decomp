#!/usr/bin/env python3
"""Store MP camera transition replay with exact Store dispatch-table check."""
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
        so=pathlib.Path(tmp)/"transition.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_camera_transition_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.TransitionMock.argtypes=[C.c_uint];lib.TransitionMock.restype=C.c_void_p
        lib.TransitionSize.restype=C.c_uint;lib.TransitionRun.argtypes=[C.c_void_p,*([C.c_uint]*3),C.c_void_p]
        a=C.create_string_buffer(lib.TransitionSize());b=C.create_string_buffer(lib.TransitionSize());rng=random.Random(0xbfcc0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            table=struct.unpack_from("<7I",mem,0xc0044)
            print("Store transition switch:",*[hex(v) for v in table],flush=True)
            assert table==(0xbfdd6,0xbfd97,0xbfd4b,0xbfd58,0xbfd60,0xbfe81,0xbfe18)
            struct.pack_into("<Q",mem,0x8a7dd8,lib.TransitionMock(0))
            for i,rva in enumerate([0x18e600,0x46cef0,0x46b300,0xbf890],1):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.TransitionMock(i))+b"\xff\xe0"
            for old in range(8):
                for new in range(8):
                    for n in range(512):
                        args=(old,new,rng.randrange(1<<14))
                        lib.TransitionRun(base,*args,a);lib.TransitionRun(None,*args,b)
                        if a.raw!=b.raw:
                            x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                            raise AssertionError((args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("32768 complete Store camera transition comparisons")
if __name__=="__main__":main()
