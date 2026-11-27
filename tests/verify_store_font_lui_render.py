#!/usr/bin/env python3
"""Replay Store LUI inline icon, text and cache paths at draw-command boundaries."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"lui_render.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_lui_render_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.LuiRenderMock.argtypes=[C.c_uint];lib.LuiRenderMock.restype=C.c_void_p;lib.LuiRenderSize.restype=C.c_uint
        lib.LuiRenderRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.LuiRenderRun.restype=None
        lib.LuiRenderDiagnostics();a=C.create_string_buffer(lib.LuiRenderSize());b=C.create_string_buffer(lib.LuiRenderSize());rng=random.Random(0x6af8f0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dc0,0x8a82b0,0xf2040,0x71dd30,0x723450,0x71a4e0,0x71a7a0,0x3e91a0,0x69ea10,0x69bb00,0x69bd00,0x663840,0x662630]):
                ptr=lib.LuiRenderMock(i)
                if i<2:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            struct.pack_into("<Q",mem,0x8a7a60,lib.LuiRenderMock(13))
            errors=[]
            for op in range(6):
                for n in range(1024):
                    seed=rng.getrandbits(32)
                    lib.LuiRenderRun(base,op,seed,a);lib.LuiRenderRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                        break
                else:print(f"1024 Store LUI render comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
