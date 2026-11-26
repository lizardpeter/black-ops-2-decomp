#!/usr/bin/env python3
"""Full Store MP common registration replay with mocked engine call boundaries."""
import ctypes as C
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import map_image, ROOT

TARGETS=[0x46b8b0,0x46c1a0,0x46bf10,0x46c4c0,0x46c650,0x46ca30,0x46caf0,0x46cdf0,0x2983a0,0x297eb0]
GLOBALS=[0x1f04878,0x1f04908,0x1f03820,0x1f04910,0x1f04998,0x1f04850,0x1f049d8,0x1f037a8,0x1f037c8,0x1f04858,0x1f04848,0x1f02f60,0x1f04870,0x1f02f70,0x1f04840,0x1f02f50,0x1f037c0,0x1f04950,0x1f037a0,0x1f03798,0x1f04920,0x1f037e8,0x1f037d0,0x1f02f68,0x1f04868,0x1f048e8,0x1f04930,0x1f04888,0x1f049e8,0x1f04990,0x1f048f0,0x1f04938,0x1f04948,0x1f04958,0x1f02f48,0x1f037d8,0x2126558,0x2155d00,0x1f037e0,0x1f02ef0,0x1f037b8,0x1f03830,0x1f03818,0x1f04900,0x1f02f58,0x1f03808,0x1f049a0,0x1f02ef8,0x1f048e0,0x1f048d8,0x1f048c8,0x1f048c0,0x1f04880,0x1f049b8,0x1f03810,0x1f03838,0x1f037f0,0x1f02f00,0x1f037b0,0x1f037f8,0x1f02f08,0x1f049a8,0x1f049b0,0x1f049d0,0x1f049c0,0x1f049c8,0x1f048d0,0x1f049f0,0x1de1438,0x1f04988,0x1f04898,0x1f048a0,0x1f048a8,0x1f048b0,0x1f048b8]

def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"common.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_common_registration_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.CommonMock.argtypes=[C.c_uint]
        lib.CommonMock.restype=C.c_void_p
        lib.CommonSize.restype=C.c_uint
        lib.CommonRun.argtypes=[C.c_void_p,C.c_uint,C.c_void_p]
        lib.CommonRun.restype=None
        lib.CommonGlobals.argtypes=[C.c_void_p]
        lib.CommonGlobals.restype=C.c_uint
        old=C.create_string_buffer(lib.CommonSize())
        new=C.create_string_buffer(lib.CommonSize())
        globals_=(C.c_uint64*len(GLOBALS))()
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.CommonMock(i))+b"\xff\xe0"
            struct.pack_into("<Q",mem,0x8a7dc0,lib.CommonMock(10))
            for seed in range(256):
                for rva in GLOBALS:
                    struct.pack_into("<Q",mem,rva,0)
                lib.CommonRun(base,seed,old)
                lib.CommonRun(None,seed,new)
                if old.raw!=new.raw:
                    pos=next(i for i,(a,b) in enumerate(zip(old.raw,new.raw)) if a!=b)
                    if pos<8:
                        raise AssertionError((seed,"event count",struct.unpack_from("<Q",old.raw),struct.unpack_from("<Q",new.raw)))
                    idx=(pos-8)//64
                    raise AssertionError((seed,idx,struct.unpack_from("<8Q",old.raw,8+64*idx),struct.unpack_from("<8Q",new.raw,8+64*idx)))
                assert lib.CommonGlobals(globals_)==75
                assert list(globals_)==[struct.unpack_from("<Q",mem,rva)[0] for rva in GLOBALS],seed
        print("256 complete common registration replays; 145/146 ordered events and 75 result slots per case")

if __name__=="__main__":
    main()
