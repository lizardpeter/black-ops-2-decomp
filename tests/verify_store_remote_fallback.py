#!/usr/bin/env python3
"""Original Store alternate remote-eye body differential replay."""
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
        so=pathlib.Path(tmp)/"remote.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_remote_fallback_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.RemoteMock.argtypes=[C.c_uint];lib.RemoteMock.restype=C.c_void_p
        lib.RemoteSize.restype=C.c_uint;lib.RemoteRun.argtypes=[C.c_void_p,C.c_void_p,C.c_uint,C.c_int,C.c_void_p]
        a=C.create_string_buffer(lib.RemoteSize());b=C.create_string_buffer(lib.RemoteSize());rng=random.Random(0xc3650)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0xa9960,0x39aeb0,0x50a9b0,0xe6370,0xde970,0x462d70,0x622b70,0xc33f0,0x8a7dc0]):
                ptr=lib.RemoteMock(i)
                if i==8:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for n in range(16384):
                k=rng.randrange(1<<15)
                if n%2:k=(k|2|64)&~16
                args=(k,rng.choice([0,1,127,128,129,1000,-1,2147483647,-2147483648]))
                lib.RemoteRun(base,base,*args,a);lib.RemoteRun(None,base,*args,b)
                if a.raw!=b.raw:
                    x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                    raise AssertionError((n,args,[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
            print("16384 complete Store alternate remote-eye comparisons")
if __name__=="__main__":main()
