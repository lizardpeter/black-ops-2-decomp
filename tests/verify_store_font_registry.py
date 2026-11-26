#!/usr/bin/env python3
"""Replay HKS registry and allocator boundaries used by original LUI quad caching."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"registry.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_registry_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.RegistryMock.argtypes=[C.c_uint];lib.RegistryMock.restype=C.c_void_p;lib.RegistrySize.restype=C.c_uint
        lib.RegistryRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.RegistryRun.restype=None
        a=C.create_string_buffer(lib.RegistrySize());b=C.create_string_buffer(lib.RegistrySize());rng=random.Random(0x69bb00)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x680e50,0x6922c0,0x692140,0x665ca0,0x67fd00,0x67fc40,0x68f740,0x68f7d0,0x692690]):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.RegistryMock(i))+b"\xff\xe0"
            errors=[]
            for op in range(13):
                for n in range(1024):
                    seed=rng.getrandbits(32)
                    lib.RegistryRun(base,op,seed,a);lib.RegistryRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                        break
                else:print(f"1024 Store HKS registry comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
