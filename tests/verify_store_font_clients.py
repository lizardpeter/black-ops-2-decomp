#!/usr/bin/env python3
"""Replay Store HUD client branches, chat render boundary and static work tables."""
import ctypes as C
import random,struct,subprocess,sys,tempfile,pathlib
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"clients.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_clients_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.ClientsMock.argtypes=[C.c_uint];lib.ClientsMock.restype=C.c_void_p;lib.ClientsSize.restype=C.c_uint
        lib.ClientsRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.ClientsRun.restype=None
        lib.StaticBase.argtypes=[C.c_void_p]
        a=C.create_string_buffer(lib.ClientsSize());b=C.create_string_buffer(lib.ClientsSize());rng=random.Random(0xde7b0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem));lib.StaticBase(base)
            struct.pack_into("<Q",mem,0x8a7dc0,lib.ClientsMock(0))
            for i,rva in enumerate([0x46aa10,0x3e8cc0,0x472000,0x3fcd20,0x1f65e0,0x71c880,0x71a7a0,0x1a43a0],1):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.ClientsMock(i))+b"\xff\xe0"
            errors=[]
            for op in range(5):
                for n in range(1024):
                    seed=rng.getrandbits(32);lib.ClientsRun(base,op,seed,a);lib.ClientsRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        errors.append((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]));break
                else:print(f"1024 Store HUD client/table comparisons for operation {op}",flush=True)
            if errors:raise AssertionError(errors)
if __name__=="__main__":main()
