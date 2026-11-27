#!/usr/bin/env python3
"""Original-PE post-worker fragment replay; reconstructs both flashlight modes."""
import ctypes as C
import pathlib
import struct
import subprocess
import sys
import tempfile

from verify_store_cgame_registration import map_image, ROOT
from verify_store_renderer_prefix import epilogue, TARGETS

ENTRY, START, END = 0x70e2b5, 0x70face, 0x710c00
GLOBALS = [0x27e7158, 0x27e7150, 0x27e70c8, 0x27e6e78, 0x27e7128, 0x27e7198, 0x27e6c18, 0x27e6e38, 0x27e6c90, 0x27e6dc0, 0x27e6d00, 0x27e6d58, 0x27e6c88, 0x27e6e08, 0x27e7218, 0x27e6c68, 0x27e6da8, 0x27e71e8, 0x27e71f8, 0x27e71f0, 0x27e7210, 0x266d4c8, 0x266d4b0, 0x27e6be8, 0x27e6b88]


def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"post.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_renderer_prefix_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.RendererPrefixMock.argtypes=[C.c_uint]
        lib.RendererPrefixMock.restype=C.c_void_p
        lib.RendererPrefixSize.restype=C.c_uint
        lib.RendererPostWorkerRun.argtypes=[C.c_void_p,C.c_uint,C.c_uint,C.c_void_p]
        lib.RendererPostWorkerRun.restype=None
        lib.RendererPostWorkerGlobals.argtypes=[C.c_void_p]
        lib.RendererPostWorkerGlobals.restype=None
        old=C.create_string_buffer(lib.RendererPrefixSize())
        new=C.create_string_buffer(lib.RendererPrefixSize())
        values=(C.c_uint64*len(GLOBALS))()
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(i if i<7 else i+1))+b"\xff\xe0"
            mem[0x8706c0:0x8706c1]=b"\xc3"
            assert mem[ENTRY:ENTRY+5]==bytes.fromhex("e8 76 54 05 00")
            assert mem[START:START+7]==bytes.fromhex("48 8d 0d 0b c3 29 00")
            assert mem[END:END+5]==bytes.fromhex("44 89 64 24 60")
            # Original nonvolatile register values live across the prior
            # worker calls. Restore only the inputs consumed by this fragment.
            stub=b"\x48\x8d\x1d"+struct.pack("<i",0x8e1fd0-(ENTRY+7))+b"\x45\x33\xe4"
            for reg,bits in [(8,0),(9,0x3f800000),(10,0x41200000),(14,0x42480000),(15,0x42c80000)]:
                stub+=b"\xb8"+struct.pack("<I",bits)+bytes((0x66,0x44,0x0f,0x6e,0xc0|((reg&7)<<3)))
            stub+=b"\xe9"+struct.pack("<i",START-(ENTRY+len(stub)+5))
            mem[ENTRY:ENTRY+len(stub)]=stub
            tail=epilogue()
            mem[END:END+len(tail)]=tail
            masks=sorted({0,(1<<17)-1,*[1<<i for i in range(17)],
                          *[(i*0x9e3779b9)&((1<<17)-1) for i in range(1024)]})
            for mode in range(2):
                for mask in masks:
                    mem[0x27e7331]=mode
                    for rva in GLOBALS: struct.pack_into("<Q",mem,rva,0)
                    lib.RendererPostWorkerRun(base,mask,mode,old)
                    lib.RendererPostWorkerRun(None,mask,mode,new)
                    assert struct.unpack_from("<Q",old.raw)[0]==49
                    if old.raw!=new.raw:
                        pos=next(i for i,(x,y) in enumerate(zip(old.raw,new.raw)) if x!=y)
                        idx=(pos-8)//80
                        raise AssertionError((mode,mask,idx,struct.unpack_from("<10Q",old.raw,8+80*idx),struct.unpack_from("<10Q",new.raw,8+80*idx)))
                    lib.RendererPostWorkerGlobals(values)
                    assert list(values)==[struct.unpack_from("<Q",mem,rva)[0] for rva in GLOBALS],(mode,mask)
            print(f"{len(masks)*2} post-worker fragment cases; 49 ordered events and 25 globals per case")
if __name__=="__main__":
    main()
