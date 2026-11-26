#!/usr/bin/env python3
"""Compare Store input command bodies, including ordered dependency calls."""
import ctypes as C
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"commands.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_input_commands_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CommandsMock.argtypes=[C.c_uint];lib.CommandsMock.restype=C.c_void_p
        lib.CommandsSize.restype=C.c_uint
        lib.CommandsRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.CommandsRun.restype=None
        a=C.create_string_buffer(lib.CommandsSize());b=C.create_string_buffer(lib.CommandsSize())
        rng=random.Random(0x193fe0)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x193ed0,0x419ac0,0x42e5d0,0x197520,0x4194c0,0x8a7dd8,0x18e200]):
                ptr=lib.CommandsMock(i)
                if i==5:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in range(4):
                for n in range(8192):
                    seed=rng.randrange(1<<22)
                    lib.CommandsRun(base,op,seed,a);lib.CommandsRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        diffs=[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y]
                        raise AssertionError((op,n,hex(seed),diffs[:30]))
                print(f"8192 Store input command comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
