#!/usr/bin/env python3
"""Isolated replay of Store image memory helpers including the original jump tables."""
import ctypes as C
import mmap
import pathlib
import random
import struct
import subprocess
import sys
import tempfile
from verify_store_image_builtin import PE, BUILDS, ROOT

RVA = {'mp':0x776ea0, 'zm':0x7749e0, 'sp':0x61c790}

def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'memory.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_memory.cpp'),str(ROOT/'tests/store_image_memory_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so))
        u=C.c_uint; p=C.c_void_p
        lib.ReplayMip.argtypes=[p,u,u,u,u]; lib.ReplayMip.restype=u
        lib.Store_ImageMipBytes.argtypes=[u,u,u,u]; lib.Store_ImageMipBytes.restype=u
        lib.ReplayTrack.argtypes=[p,p,u,u,u,u,u]; lib.ReplayTrack.restype=None
        lib.Store_ImageTrackMemory.argtypes=[p,u,u,u,u,u]; lib.Store_ImageTrackMemory.restype=None
        normalized=None
        for mode,a in RVA.items():
            pe=PE(folder/('t6'+mode+'.exe'),BUILDS[mode][0])
            # Both routines, including the leaf's RVA jump table, are contiguous.
            # Its RIP-relative image-base LEA self-relocates to (mapping - a).
            code=pe.read(a,0x250)
            norm=bytearray(code)
            norm[0x150+0x12:0x150+0x16]=bytes(4)
            for off in (0x16b,0x173):
                struct.pack_into("<I",norm,off,struct.unpack_from("<I",code,off)[0]-a)
            for i in range(7):
                off=0x150+0x94+4*i
                struct.pack_into('<I',norm,off,struct.unpack_from('<I',code,off)[0]-a)
            if normalized is not None: assert norm==normalized,mode
            normalized=norm
            mem=mmap.mmap(-1,0x1000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
            mem[:len(code)]=code
            base=C.addressof(C.c_char.from_buffer(mem)); leaf=base+0x150
            rng=random.Random(0x776EA0)
            dims=[(1,1,1),(0,0,0),(3,5,7),(4,4,4),(5,7,1),(65535,65535,65535),(0xffffffff,1,4)]+[tuple(rng.randrange(1,16385) for _ in range(3)) for _ in range(128)]
            mip_count=0
            for f in range(100):
                for w,h,d in dims:
                    assert lib.ReplayMip(leaf,f,w,h,d)==lib.Store_ImageMipBytes(f,w,h,d),(mode,f,w,h,d)
                    mip_count+=1
            track_count=0
            for f in range(100):
                for flags in range(16):
                    for w,h,d in [(1,1,1),(3,5,7),(64,32,1),(65535,65535,1),tuple(rng.randrange(1,1025) for _ in range(3))]:
                        data=bytearray(rng.randbytes(64));data[12]=rng.randrange(256);data[13]=rng.randrange(256)
                        x=C.create_string_buffer(bytes(data),64);y=C.create_string_buffer(bytes(data),64)
                        lib.ReplayTrack(base,x,flags,f,w,h,d);lib.Store_ImageTrackMemory(y,flags,f,w,h,d)
                        assert x.raw==y.raw,(mode,f,flags,w,h,d,x.raw.hex(),y.raw.hex())
                        track_count+=1
            mem.close()
            print(f'{mode}: {mip_count} mip-size and {track_count} memory-accounting comparisons passed')
if __name__=='__main__': main()
