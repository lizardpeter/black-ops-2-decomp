#!/usr/bin/env python3
"""Byte-comparison of original Store render command and rectangle transforms."""
import ctypes as C
import pathlib,random,struct,subprocess,sys,tempfile
from verify_store_cgame_registration import ROOT,map_image
def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"textcmd.so"
        subprocess.run(["g++","-std=c++17","-O2","-ffp-contract=off","-Wall","-Wextra","-Werror","-shared","-fPIC",str(ROOT/"tests/store_font_command_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so));lib.CmdMock.argtypes=[C.c_uint];lib.CmdMock.restype=C.c_void_p
        lib.CmdSize.restype=C.c_uint;lib.CmdRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p];lib.CmdRun.restype=None
        a=C.create_string_buffer(lib.CmdSize());b=C.create_string_buffer(lib.CmdSize());rng=random.Random(0x71c880)
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate([0x8a7dc0,0xf2040,0x870aa2]):
                ptr=lib.CmdMock(i)
                if i==0:struct.pack_into("<Q",mem,rva,ptr)
                else:mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",ptr)+b"\xff\xe0"
            for op in range(2):
                for n in range(8192):
                    seed=rng.getrandbits(32)
                    lib.CmdRun(base,op,seed,a);lib.CmdRun(None,op,seed,b)
                    if a.raw!=b.raw:
                        raise AssertionError((op,n,hex(seed),[(i,x,y) for i,(x,y) in enumerate(zip(a.raw,b.raw)) if x!=y][:40]))
                print(f"8192 Store text command/rect comparisons for operation {op}",flush=True)
if __name__=="__main__":main()
