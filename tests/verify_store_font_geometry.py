#!/usr/bin/env python3
"""Compare original tessellation vertex bytes, index bytes, flushes and cache state."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"geometry.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_geometry_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.GeometryMock.restype=C.c_void_p;lib.GeometrySize.restype=C.c_uint
        lib.GeometryRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.GeometryRun.restype=None
        a=C.create_string_buffer(lib.GeometrySize());b=C.create_string_buffer(lib.GeometrySize());rng=random.Random(0x72bfe0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            mem[0x73d680:0x73d68c]=b"\x48\xb8"+struct.pack("<Q",lib.GeometryMock())+b"\xff\xe0"
            for op in range(4):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.GeometryRun(base,op,seed,a);lib.GeometryRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store glyph geometry comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
