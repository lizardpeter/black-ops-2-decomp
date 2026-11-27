#!/usr/bin/env python3
"""Compare complete Store font cache and HUD scale source bodies."""
import ctypes as C
import pathlib, random, struct, subprocess, sys, tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"font.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.FontMock.argtypes=[C.c_uint];lib.FontMock.restype=C.c_void_p
        lib.FontSize.restype=C.c_uint;lib.FontRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.FontRun.restype=None
        a=C.create_string_buffer(lib.FontSize());b=C.create_string_buffer(lib.FontSize());rng=random.Random(0x3fbaf0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dc0,0x472000,0x71a780,0x71a4e0]):
                ptr=lib.FontMock(i)
                if i==0:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in range(6):
                for n in range(8192):
                    seed=rng.randrange(1<<27)
                    lib.FontRun(base,op,seed,a);lib.FontRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        x=struct.unpack("<"+"Q"*(len(a.raw)//8),a.raw);y=struct.unpack("<"+"Q"*(len(b.raw)//8),b.raw)
                        raise AssertionError((op,n,hex(seed),[(i,hex(v),hex(w)) for i,(v,w) in enumerate(zip(x,y)) if v!=w][:30]))
                print(f"8192 Store font comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
