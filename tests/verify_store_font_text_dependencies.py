#!/usr/bin/env python3
"""Replay Store text binding lookup and cached tag positioning dependencies."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"textdep.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_text_dependencies_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.TextDependencyMock.argtypes=[C.c_uint];lib.TextDependencyMock.restype=C.c_void_p;lib.TextDependencySize.restype=C.c_uint
        lib.TextDependencyRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.TextDependencyRun.restype=None
        a=C.create_string_buffer(lib.TextDependencySize());b=C.create_string_buffer(lib.TextDependencySize());rng=random.Random(0x106c50)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x1a5060,0x3a5e70,0x471e50,0x106ba0,0xe6370,0x1bbbc0,0x4a1290,0x122180,0x4a3850]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.TextDependencyMock(i))+b"\xff\xe0"
            for op in range(6):
                for n in range(2048):
                    seed=rng.getrandbits(32);lib.TextDependencyRun(base,op,seed,a);lib.TextDependencyRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"2048 Store text dependency comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
