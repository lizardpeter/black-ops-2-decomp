#!/usr/bin/env python3
"""Replay original interned string lookup and LUI VM string insertion."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"string.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_string_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.StringMock.argtypes=[C.c_uint];lib.StringMock.restype=C.c_void_p;lib.StringSize.restype=C.c_uint
        lib.StringRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.StringRun.restype=None
        a=C.create_string_buffer(lib.StringSize());b=C.create_string_buffer(lib.StringSize());rng=random.Random(0x50a9b0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in [(0,0x50a930),(4,0x697d00)]:
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.StringMock(i))+b"\xff\xe0"
            for i,rva in [(1,0x8a72f0),(2,0x8a72f8)]:
                struct.pack_into("<Q",mem,rva,lib.StringMock(i))
            rva=0x870ab4
            assert mem[rva:rva+2]==b"\xff\x25"
            struct.pack_into("<Q",mem,rva+6+struct.unpack_from("<i",mem,rva+2)[0],lib.StringMock(3))
            for op in range(2):
                for n in range(8192):
                    seed=rng.getrandbits(32);lib.StringRun(base,op,seed,a);lib.StringRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store string comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
