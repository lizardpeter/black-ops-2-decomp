#!/usr/bin/env python3
"""Compare complete Store main view body, with explicit external call stubs."""
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
        so=pathlib.Path(tmp)/"main-view.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_main_view_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.MainViewMock.argtypes=[C.c_uint];lib.MainViewMock.restype=C.c_void_p
        lib.MainViewSize.restype=C.c_uint;lib.MainViewRun.argtypes=[C.c_void_p,*([C.c_uint]*3),C.c_void_p]
        old=C.create_string_buffer(lib.MainViewSize());new=C.create_string_buffer(lib.MainViewSize())
        rng=random.Random(0x16C510)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            targets=[0xc0e20,0xc2060,0x173ed0,0x16bda0,0x464d00,0x100fc0,0x16b450,0xc4490,0xc5fa0,0xc3ae0,0xc3390,0x621c10,0xc3950,0x16bfb0,0xc2740,0x16ba40,0xc66d0,0xc7540,0x16c380,0xbf940,0xc3e10,0x6468b0,0x6448b0,0x17f0b0,0xe6370,0xde970,0x462d70,0x16a940,0x157bc0]
            for i,rva in enumerate(targets):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.MainViewMock(i))+b"\xff\xe0"
            for i,rva in enumerate([0x8a7dc0,0x8a7dd8],len(targets)):
                struct.pack_into("<Q",mem,rva,lib.MainViewMock(i))
            for case in range(16384):
                key=rng.randrange(1<<24)
                # Exercise the full view path frequently, as well as cubemap,
                # demo free-view and linked-view exits.
                if case%2:key &= ~((1<<9)|(1<<16)|(1<<17))
                args=(key,rng.randrange(1<<25),rng.randrange(1<<32))
                lib.MainViewRun(base,*args,old);lib.MainViewRun(None,*args,new)
                if old.raw!=new.raw:
                    x=struct.unpack("<"+str(len(old.raw)//8)+"Q",old.raw)
                    y=struct.unpack("<"+str(len(new.raw)//8)+"Q",new.raw)
                    raise AssertionError((case,args,[(i,hex(a),hex(b)) for i,(a,b) in enumerate(zip(x,y)) if a!=b][:30]))
            print("16384 complete Store CG_CalcViewValues comparisons; 52 output words and ordered calls, client 0")
if __name__=="__main__":main()
