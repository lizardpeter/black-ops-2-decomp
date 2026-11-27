#!/usr/bin/env python3
"""Isolated original renderer worker registration fragment differential replay."""
import ctypes as C
import itertools
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import map_image, ROOT
from verify_store_renderer_prefix import epilogue, TARGETS

ENTRY = 0x70e2b5
START = 0x70f91f
END = 0x70face
GLOBALS = [0x27e71e0] + [0x27e6eb0+8*i for i in range(8)]


def main():
    with tempfile.TemporaryDirectory() as tmp:
        so = pathlib.Path(tmp)/"worker.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_renderer_prefix_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.RendererPrefixMock.argtypes=[C.c_uint]
        lib.RendererPrefixMock.restype=C.c_void_p
        lib.RendererPrefixSize.restype=C.c_uint
        lib.RendererWorkerRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_uint,C.c_int,C.c_void_p]
        lib.RendererWorkerRun.restype=None
        lib.RendererWorkerGlobals.argtypes=[C.c_void_p]
        lib.RendererWorkerGlobals.restype=None
        old=C.create_string_buffer(lib.RendererPrefixSize())
        new=C.create_string_buffer(lib.RendererPrefixSize())
        slots=(C.c_uint64*9)()
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(i if i<7 else i+1))+b"\xff\xe0"
            mem[0x8706c0:0x8706c1]=b"\xc3"
            struct.pack_into("<Q",mem,0x8a7dc0,lib.RendererPrefixMock(7))
            # Initialize RBX to the original empty string, R12D to zero;
            # retain the genuine prologue and use the original fragment.
            assert mem[ENTRY:ENTRY+5]==bytes.fromhex("e8 76 54 05 00")
            stub=b"\x48\x8d\x1d"+struct.pack("<i",0x8e1fd0-(ENTRY+7))+b"\x45\x33\xe4"
            stub+=b"\xe9"+struct.pack("<i",START-(ENTRY+len(stub)+5))
            mem[ENTRY:ENTRY+len(stub)]=stub
            tail=epilogue()
            mem[END:END+len(tail)]=tail
            # The mapper does not relocate absolute pointer data.
            for i in range(8):
                addr,=struct.unpack_from("<Q",mem,0x9ad3f0+8*i)
                assert 0x140000000<=addr<0x141000000
                struct.pack_into("<Q",mem,0x9ad3f0+8*i,base+addr-0x140000000)
            count=0
            for exists,hardware,kind,value in itertools.product(range(2),[0,1,2,3,4,5,6,7,8,9,10,11,64,0xffffffff],[1,2,6,7,8],range(-1,9)):
                for rva in GLOBALS: struct.pack_into("<Q",mem,rva,0)
                struct.pack_into("<I",mem,0x406fc3c,hardware)
                lib.RendererWorkerRun(base,exists,hardware,kind,value,old)
                lib.RendererWorkerRun(None,exists,hardware,kind,value,new)
                assert old.raw==new.raw,(exists,hardware,kind,value)
                lib.RendererWorkerGlobals(slots)
                assert list(slots)==[struct.unpack_from("<Q",mem,rva)[0] for rva in GLOBALS],(exists,hardware,kind,value)
                count+=1
            print(f"{count} worker fragment replays; hardware thresholds, five value types and both lookup branches")
if __name__=="__main__":
    main()
