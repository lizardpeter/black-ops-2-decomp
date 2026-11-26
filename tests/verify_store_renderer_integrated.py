#!/usr/bin/env python3
"""Continuous original-PE replay of the entire recovered renderer prefix.

Unlike the isolated fragments, this runs the original prologue and every
instruction between entry and 140710C00, using only the existing mocked
registration, initialization, stack-probe and atoi boundaries.
"""
import ctypes as C
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import map_image, ROOT
from verify_store_renderer_prefix import epilogue, TARGETS, GLOBALS as EARLY
from verify_store_renderer_workers import GLOBALS as WORKERS
from verify_store_renderer_post_worker import GLOBALS as LATE, END


def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"renderer-integrated.so"
        subprocess.run(["g++","-std=c++17","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_renderer_prefix_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.RendererPrefixMock.argtypes=[C.c_uint]
        lib.RendererPrefixMock.restype=C.c_void_p
        lib.RendererPrefixSize.restype=C.c_uint
        lib.RendererIntegratedRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_int,C.c_uint,C.c_void_p]
        lib.RendererIntegratedRun.restype=None
        groups=[(lib.RendererPrefixGlobals,EARLY),(lib.RendererWorkerGlobals,WORKERS),(lib.RendererPostWorkerGlobals,LATE)]
        for get,_ in groups:
            get.argtypes=[C.c_void_p]
            get.restype=None
        old=C.create_string_buffer(lib.RendererPrefixSize())
        new=C.create_string_buffer(lib.RendererPrefixSize())
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(i if i<7 else i+1))+b"\xff\xe0"
            mem[0x8706c0:0x8706c1]=b"\xc3"
            struct.pack_into("<Q",mem,0x8a7dc0,lib.RendererPrefixMock(7))
            assert mem[END:END+5]==bytes.fromhex("44 89 64 24 60")
            tail=epilogue()
            mem[END:END+len(tail)]=tail
            for i in range(8):
                addr,=struct.unpack_from("<Q",mem,0x9ad3f0+8*i)
                assert 0x140000000<=addr<0x141000000
                struct.pack_into("<Q",mem,0x9ad3f0+8*i,base+addr-0x140000000)
            for seed in range(512):
                early=(seed*0x9e3779b9)&((1<<18)-1)
                late=(seed*0x85ebca6b)&((1<<17)-1)
                exists=(seed>>1)&1
                mode=seed&1
                hardware=[0,2,3,4,5,10,11,64,0xffffffff][(seed//4)%9]
                kind=[1,2,6,7,8][(seed//7)%5]
                value=(seed//3)%10-1
                for _,rvas in groups:
                    for rva in rvas: struct.pack_into("<Q",mem,rva,0)
                mem[0x27e7331]=mode
                struct.pack_into("<I",mem,0x406fc3c,hardware)
                args=(early,late,exists,hardware,kind,value,mode)
                lib.RendererIntegratedRun(base,*args,old)
                lib.RendererIntegratedRun(None,*args,new)
                workers=int(value!=0) if kind==1 else max(0,value)
                assert struct.unpack_from("<Q",old.raw)[0]==153+workers
                if old.raw!=new.raw:
                    pos=next(i for i,(x,y) in enumerate(zip(old.raw,new.raw)) if x!=y)
                    idx=(pos-8)//80
                    raise AssertionError((seed,args,idx,struct.unpack_from("<10Q",old.raw,8+80*idx),struct.unpack_from("<10Q",new.raw,8+80*idx)))
                for get,rvas in groups:
                    values=(C.c_uint64*len(rvas))()
                    get(values)
                    assert list(values)==[struct.unpack_from("<Q",mem,rva)[0] for rva in rvas],(seed,args,rvas)
            print("512 continuous renderer prefix replays; 153..161 ordered events, 92 global slots")
if __name__=="__main__":
    main()
