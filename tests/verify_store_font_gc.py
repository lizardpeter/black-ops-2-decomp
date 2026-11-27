#!/usr/bin/env python3
"""Replay original HKS write barriers, collection controls and metatable lookup."""
import ctypes as C
import random,struct,subprocess,sys,tempfile,pathlib
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"gc.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_gc_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.GCMock.argtypes=[C.c_uint];lib.GCMock.restype=C.c_void_p;lib.GCSize.restype=C.c_uint
        lib.GCRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.GCRun.restype=None
        a=C.create_string_buffer(lib.GCSize());b=C.create_string_buffer(lib.GCSize());rng=random.Random(0x692140)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x6918c0,0x68f6c0,0x69d7a0,0x870aa2]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.GCMock(i))+b"\xff\xe0"
            errors=[]
            for op in range(7):
                for n in range(1024):
                    seed=rng.getrandbits(32);lib.GCRun(base,op,seed,a);lib.GCRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]));break
                else:print(f"1024 Store HKS GC/metatable comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
