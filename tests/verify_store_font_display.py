#!/usr/bin/env python3
"""Replay six original Store display/resource functions."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"display.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_display_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.DisplayMock.restype=C.c_void_p;lib.DisplaySize.restype=C.c_uint
        lib.DisplayRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.DisplayRun.restype=None
        a=C.create_string_buffer(lib.DisplaySize());b=C.create_string_buffer(lib.DisplaySize());rng=random.Random(0x6aefd0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            lib.DisplayMock.argtypes=[C.c_int]
            for i,rva in enumerate((0x8a7dc0,0,0x8a7968,0x8a7928,0x8a7838,0x8a72f0,0x8a72f8)):
                if rva:struct.pack_into("<Q",mem,rva,lib.DisplayMock(i))
            mem[0x46cef0:0x46cefc]=b'\x48\xb8'+struct.pack('<Q',lib.DisplayMock(1))+b'\xff\xe0'
            for op in range(6):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.DisplayRun(base,op,seed,a);lib.DisplayRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store display/resource comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
