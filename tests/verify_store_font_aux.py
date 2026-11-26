#!/usr/bin/env python3
"""Differential comparisons of Store text wrapping, replacement and viewport transforms."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"aux.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_aux_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.AuxMock.argtypes=[C.c_uint];lib.AuxMock.restype=C.c_void_p;lib.AuxSize.restype=C.c_uint
        lib.AuxRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.AuxRun.restype=None
        a=C.create_string_buffer(lib.AuxSize());b=C.create_string_buffer(lib.AuxSize());rng=random.Random(0x71a9c0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a82b0,0xf27a0,0x870aa2,0x8a79f0,0x8a7dd8]):
                ptr=lib.AuxMock(i)
                if i in (0,3,4):struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in [0,1,2,3,4,5]:
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.AuxRun(base,op,seed,a);lib.AuxRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store auxiliary font/state comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
