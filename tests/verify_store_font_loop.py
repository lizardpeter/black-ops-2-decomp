#!/usr/bin/env python3
"""Replay original Store DrawTeleType with captured backend calls."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"textloop.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_loop_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.LoopMock.argtypes=[C.c_uint];lib.LoopMock.restype=C.c_void_p;lib.LoopSize.restype=C.c_uint
        lib.LoopRun.argtypes=[C.c_void_p,C.c_uint,C.c_void_p];lib.LoopRun.restype=None
        lib.LoopRun2D.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.LoopRun2D.restype=None
        a=C.create_string_buffer(lib.LoopSize());b=C.create_string_buffer(lib.LoopSize());rng=random.Random(0x734690)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x71a7a0,0x72bfe0,0x732260,0x731e00,0x7320e0]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.LoopMock(i))+b"\xff\xe0"
            struct.pack_into("<Q",mem,0x8a7ba0,lib.LoopMock(5))
            for i,rva in [(6,0x3e9910),(7,0x72b8f0),(8,0x7323b0)]:
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.LoopMock(i))+b"\xff\xe0"
            for n in range(16384):
                seed=rng.getrandbits(32)
                lib.LoopRun(base,seed,a);lib.LoopRun(None,seed,b)
                if a.raw!=b.raw:raise AssertionError((n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
            print("16384 Store DrawTeleType differential comparisons",flush=True)
            for mode in (1,2):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.LoopRun2D(base,seed,mode,a);lib.LoopRun2D(None,seed,mode,b)
                    if a.raw!=b.raw:raise AssertionError((mode,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store DrawText2D comparisons for mode {mode}",flush=True)
if __name__=="__main__":main()
