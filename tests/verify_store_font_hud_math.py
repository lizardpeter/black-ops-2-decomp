#!/usr/bin/env python3
"""Original Store HUD projection, color, clamp, fade and axis replay."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"hudmath.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_hud_math_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.HudMock.argtypes=[C.c_uint];lib.HudMock.restype=C.c_void_p;lib.HudSize.restype=C.c_uint
        lib.HudRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.HudRun.restype=None
        a=C.create_string_buffer(lib.HudSize());b=C.create_string_buffer(lib.HudSize());rng=random.Random(0xf4c00)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dc0,0x8a7dd8]):struct.pack_into("<Q",mem,rva,lib.HudMock(i))
            errors=[]
            for op in range(6):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.HudRun(base,op,seed,a);lib.HudRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                        break
                else:print(f"8192 Store HUD math comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
