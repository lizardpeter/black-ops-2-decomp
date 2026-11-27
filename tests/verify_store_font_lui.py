#!/usr/bin/env python3
"""Replay six original Store LUI utility functions."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"lui.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_lui_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.LuiMock.restype=C.c_void_p;lib.LuiSize.restype=C.c_uint
        lib.LuiRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.LuiRun.restype=None
        a=C.create_string_buffer(lib.LuiSize());b=C.create_string_buffer(lib.LuiSize());rng=random.Random(0x6aefd0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem));struct.pack_into("<Q",mem,0x8a7dc0,lib.LuiMock())
            for op in range(6):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.LuiRun(base,op,seed,a);lib.LuiRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store LUI utility comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
