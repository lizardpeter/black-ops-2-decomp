#!/usr/bin/env python3
"""Replay original Store asset hash chains, default asset cloning and CSV cache."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"asset.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_asset_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.AssetMock.argtypes=[C.c_uint];lib.AssetMock.restype=C.c_void_p;lib.AssetSizeCapture.restype=C.c_uint
        lib.AssetRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.AssetRun.restype=None
        a=C.create_string_buffer(lib.AssetSizeCapture());b=C.create_string_buffer(lib.AssetSizeCapture());rng=random.Random(0x211b80)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem));struct.pack_into("<Q",mem,0x8a8298,lib.AssetMock(0))
            for i,rva in [(1,0x471c80),(5,0x211c60),(6,0x2116e0),(7,0x50abe0),(8,0x212150),(9,0x466400),(10,0x870aa2),(11,0x870aa8)]:
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.AssetMock(i))+b"\xff\xe0"
            errors=[]
            for op in range(4):
                for n in range(4096):
                    seed=rng.getrandbits(32);lib.AssetRun(base,op,seed,a);lib.AssetRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]));break
                else:print(f"4096 Store asset comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
