#!/usr/bin/env python3
"""Original Store fastfile zone-name and synchronous load replay."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"zones.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_zones_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ZoneMock.argtypes=[C.c_uint];lib.ZoneMock.restype=C.c_void_p;lib.ZoneSize.restype=C.c_uint
        lib.ZoneBind.argtypes=[C.c_void_p];lib.ZoneRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.ZoneRun.restype=None
        a=C.create_string_buffer(lib.ZoneSize());b=C.create_string_buffer(lib.ZoneSize());rng=random.Random(0x215300)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem));lib.ZoneBind(base)
            for i,rva in [(0,0x8a7dc0),(2,0x8a7250)]:struct.pack_into("<Q",mem,rva,lib.ZoneMock(i))
            for i,rva in [(1,0x472000),(3,0x49b680),(4,0x213d70),(5,0x71e160),(6,0x71e200),(7,0x213410)]:
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.ZoneMock(i))+b"\xff\xe0"
            for op in range(3):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.ZoneRun(base,op,seed,a);lib.ZoneRun(None,op,seed,b)
                    if a.raw!=b.raw:raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store zone comparisons for operation {op}",flush=True)
            for resolution in (720,1080,2160):
                # Select integer/enum representation of the resolution value.
                seed=1000+resolution
                assert seed%6 in (3,4)
                lib.ZoneRun(base,0,seed,a)
                print("Original Store zone strings for",resolution,
                      [a.raw[i*24:(i+1)*24].split(b"\x00",1)[0].decode("ascii") for i in range(13)],flush=True)
if __name__=="__main__":main()
