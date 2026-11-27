#!/usr/bin/env python3
"""Replay the complete Store renderer registration routine, with its genuine epilogue."""
import ctypes as C
import pathlib
import struct
import subprocess
import sys
import tempfile
from verify_store_cgame_registration import map_image, ROOT
from verify_store_renderer_prefix import epilogue, TARGETS, GLOBALS as EARLY
from verify_store_renderer_workers import GLOBALS as WORKERS
from verify_store_renderer_post_worker import GLOBALS as LATE

END=0x7175d7
TAIL=[0x27e6cd8, 0x27e6d68, 0x27e6b08, 0x27e6b80, 0x27e6b98, 0x27e6f38, 0x27e6f48, 0x27e70f0, 0x27e6cb0, 0x27e6cb8, 0x27e71c0, 0x27e70a8, 0x27e7180, 0x27e6e80, 0x27e70b8, 0x27e71d8, 0x20fa628, 0x27e7120, 0x27e6ff0, 0x27e6f08, 0x27e7008, 0x27e6f90, 0x27e7100, 0x27e6e98, 0x27e7010, 0x27e71b8, 0x27e6c58, 0x27e7110, 0x27e6d60, 0x27e6d50, 0x27e6df0, 0x27e6d30, 0x27e6f78, 0x27e6f50, 0x27e7160, 0x27e6bd8, 0x27e6e88, 0x27e7068, 0x27e70c0, 0x27e6c20, 0x27e70d8, 0x27e7130, 0x27e6de0, 0x27e6e58, 0x27e7138, 0x27e6d08, 0x27e6d18, 0x27e6d48, 0x27e6e18, 0x27e6cf8, 0x27e6ce8, 0x27e7208]
VIS=[0x27e6bd0, 0x27e6bc8, 0x27e6b58, 0x27e7140, 0x27e6f58, 0x27e6fc8, 0x27e6fd8, 0x27e7250, 0x27e71a0, 0x27e6dd8, 0x27e6c78, 0x27e6f80, 0x27e6df8, 0x27e7038, 0x27e7088, 0x27e6d38, 0x27e6e90, 0x27e6e60, 0x27e7040, 0x27e6d90, 0x27e6c98, 0x27e6da0, 0x27e6ea8, 0x27e7268, 0x27e7270, 0x27e6bf0, 0x27e6f30, 0x265f2b8, 0x27e6e50, 0x27e6d70, 0x27e7060, 0x27e6ff8, 0x266d490, 0x27e7200, 0x27e7050, 0x27e6c80, 0x27e6b48, 0x27e6fe8, 0x27e6b00, 0x27e6ba8, 0x27e6b28, 0x27e6bb8, 0x27e6b78, 0x27e6b38, 0x27e6b50, 0x266d4e8, 0x27e6ba0, 0x27e7238, 0x27e6f40, 0x266d4d8, 0x27e6f20, 0x27e6b40, 0x27e6b90, 0x27e6b10, 0x27e6bc0, 0x27e6b30, 0x27e6b20, 0x27e6be0, 0x27e6b70, 0x266d4f8, 0x266d4e0, 0x27e6b68, 0x27e6b18, 0x266d4f0, 0x27e6b60, 0x27e6fb0, 0x27e7248, 0x27e6e28, 0x27e6e30, 0x27e6c60, 0x27e6d80, 0x27e6c40, 0x27e6c48, 0x27e6e48, 0x27e6cd0, 0x27e6d28, 0x27e6c30, 0x27e6c10, 0x27e6d78, 0x27e6c38, 0x27e6db0, 0x27e6ce0, 0x27e6cc0, 0x27e6ca0, 0x27e6c28, 0x27e6e20, 0x27e6c08, 0x27e6e70, 0x27e6e00, 0x266d4c0, 0x266d4a0, 0x266d498, 0x266d4a8, 0x27e6f60, 0x27e6d40, 0x266d4b8, 0x27e7260, 0x266d4d0, 0x27e7020, 0x27e6e10, 0x27e6c70, 0x27e6c00, 0x27e6dc8, 0x27e6dd0, 0x27e6bf8, 0x27e6cf0, 0x27e6db8, 0x27e7240, 0x27e6d20, 0x27e70e0, 0x27e6c50, 0x27e6de8]


def main():
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/"renderer-integrated.so"
        subprocess.run(["g++","-std=c++17","-I"+str(ROOT/"include"),"-fms-extensions","-O2","-Wall","-Wextra","-Werror","-shared","-fPIC",
                        str(ROOT/"tests/store_renderer_prefix_probe.cpp"),"-o",str(so)],check=True)
        lib=C.CDLL(str(so))
        lib.RendererPrefixMock.argtypes=[C.c_uint]
        lib.RendererPrefixMock.restype=C.c_void_p
        lib.RendererPrefixSize.restype=C.c_uint
        lib.RendererCompleteRun.argtypes=[C.c_void_p,*([C.c_uint]*5),C.c_int,C.c_uint,C.c_uint64,C.c_uint64,C.c_uint,C.c_void_p]
        lib.RendererCompleteRun.restype=None
        groups=[(lib.RendererPrefixGlobals,EARLY),(lib.RendererWorkerGlobals,WORKERS),(lib.RendererPostWorkerGlobals,LATE),(lib.RendererVisibilityGlobals,VIS),(lib.RendererFinalGlobals,TAIL)]
        for get,_ in groups:
            get.argtypes=[C.c_void_p]
            get.restype=None
        old=C.create_string_buffer(lib.RendererPrefixSize())
        new=C.create_string_buffer(lib.RendererPrefixSize())
        with map_image(pathlib.Path(sys.argv[1])) as mem:
            base=C.addressof(C.c_char.from_buffer(mem))
            for i,rva in enumerate(TARGETS):
                mem[rva:rva+12]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(i if i<7 else i+1))+b"\xff\xe0"
            mem[0x46c930:0x46c93c]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(10))+b"\xff\xe0"
            mem[0x46cef0:0x46cefc]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(11))+b"\xff\xe0"
            mem[0x46b300:0x46b30c]=b"\x48\xb8"+struct.pack("<Q",lib.RendererPrefixMock(12))+b"\xff\xe0"
            mem[0x8706c0:0x8706c1]=b"\xc3"
            struct.pack_into("<Q",mem,0x8a7dc0,lib.RendererPrefixMock(7))
            assert mem[END]==0xc3  # original return; no artificial cutoff or epilogue
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
                vis=(seed*0xd6e8feb86659fd93)&((1<<57)-1)
                tail=(seed*0x94d049bb133111eb)&((1<<53)-1)
                setcase=(seed//2)%4
                if setcase==3: tail &= ~(1<<39)
                args=(early,late,exists,hardware,kind,value,mode,vis,tail,setcase)
                lib.RendererCompleteRun(base,*args,old)
                lib.RendererCompleteRun(None,*args,new)
                workers=int(value!=0) if kind==1 else max(0,value)
                assert struct.unpack_from("<Q",old.raw)[0]==507+workers+3*mode+int(setcase<2)
                if old.raw!=new.raw:
                    pos=next(i for i,(x,y) in enumerate(zip(old.raw,new.raw)) if x!=y)
                    idx=(pos-8)//80
                    raise AssertionError((seed,args,idx,struct.unpack_from("<10Q",old.raw,8+80*idx),struct.unpack_from("<10Q",new.raw,8+80*idx)))
                for get,rvas in groups:
                    values=(C.c_uint64*len(rvas))()
                    get(values)
                    assert list(values)==[struct.unpack_from("<Q",mem,rva)[0] for rva in rvas],(seed,args,rvas)
            print("512 complete renderer replays; 507..519 ordered events, 256 global slots")
if __name__=="__main__":
    main()
