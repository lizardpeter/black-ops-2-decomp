#!/usr/bin/env python3
"""Replay the three resource constructors with mocked D3D11 COM calls."""
import ctypes as C
import mmap, pathlib, random, struct, subprocess, sys, tempfile
from verify_store_image_builtin import PE, BUILDS, ROOT
# Exact PE addresses; only the verified RIP-relative global references are relocated.
SPECS={
 'mp':(0x73b7d0,0x282e538,0x27f1d88,0x2155dfc),
 'zm':(0x739300,0x282c5b8,0x27efe08,0x2153e7c),
 'sp':(0x5de8c0,0x23bdcb8,0x2381508,0x1cce070),
}
# displacement offset, instruction end, target slot (relative to first function)
RELOCS=[(0x29,0x2e,0x1000),(0xa3,0xa7,0x1000),(0xac,0xb0,0x1004),
        (0xf4,0xf8,0x1008),(0x10c,0x110,0x1010),(0x173,0x177,0x1008),(0x18b,0x18f,0x1010),
        (0x243,0x247,0x1008),(0x25c,0x261,0x1010),(0x284,0x288,0x1008),(0x29d,0x2a2,0x1010),
        (0x343,0x347,0x1008),(0x35f,0x363,0x1010),(0x386,0x38a,0x1008),(0x3a2,0x3a6,0x1010)]
# Correct exact offsets checked against each pinned PE before executing.
class Desc(C.Structure):
    _fields_=[('words',C.c_uint*22)]
class Cap(C.Structure):
    _fields_=[('desc',Desc),('create',C.c_uint),('view',C.c_uint),('release',C.c_uint),('bad',C.c_uint)]
def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_descriptor.cpp'),str(ROOT/'tests/store_image_descriptor_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));u=C.c_uint;p=C.c_void_p
        lib.DescriptorDevice.restype=p;lib.CaptureDescriptor.argtypes=[p]+[u]*7+[p];lib.CaptureDescriptor.restype=None
        for name,n in [('2D',7),('3D',5),('Cube',3)]:
            fn=getattr(lib,'Store_DescribeImage'+name);fn.argtypes=[p]+[u]*n;fn.restype=None
        norm_ref=None
        for mode,(a,ms,device,retry) in SPECS.items():
            pe=PE(folder/('t6'+mode+'.exe'),BUILDS[mode][0]);code=bytearray(pe.read(a,0x3c8)); norm=bytearray(code)
            assert [pe.rows[a+x]-(a+x) for x in (0,0x1c0,0x2d0)]==[437,258,248]
            targets={0x1000:ms,0x1004:ms+4,0x1008:device,0x1010:retry}
            for off,end,dst in RELOCS:
                assert a+end+struct.unpack_from('<i',code,off)[0]==targets[dst],(mode,hex(off),hex(a+end+struct.unpack_from('<i',code,off)[0]),hex(targets[dst]))
                struct.pack_into('<i',code,off,dst-end);norm[off:off+4]=bytes(4)
            if norm_ref is not None: assert norm==norm_ref,mode
            norm_ref=norm
            mem=mmap.mmap(-1,0x2000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code
            base=C.addressof(C.c_char.from_buffer(mem));struct.pack_into('<Q',mem,0x1008,lib.DescriptorDevice());rng=random.Random(17);count=0
            for kind,offset in [(3,0),(4,0x1c0),(5,0x2d0)]:
                for fmt in range(100):
                    for flags in [0,0x20000,0x40000,0x60000]:
                        for samples in [0,1,2,4,8]:
                            w,h,d,levels,quality=[rng.randrange(1,100000) for _ in range(5)]
                            struct.pack_into('<II',mem,0x1000,samples,quality)
                            native=Desc();orig=Cap()
                            if kind==3: lib.Store_DescribeImage2D(C.byref(native),w,h,levels,flags,fmt,samples,quality)
                            elif kind==4: lib.Store_DescribeImage3D(C.byref(native),w,h,d,levels,fmt)
                            else: lib.Store_DescribeImageCube(C.byref(native),w,levels,fmt)
                            lib.CaptureDescriptor(base+offset,kind,w,h,d,levels,flags,fmt,C.byref(orig))
                            assert (orig.create,orig.view,orig.release,orig.bad)==(1,1,1,0),(mode,kind,orig.bad)
                            assert bytes(native)==bytes(orig.desc),(mode,kind,fmt,flags,samples,list(native.words),list(orig.desc.words))
                            count+=1
            mem.close();print(mode,count,'resource descriptor comparisons passed')
if __name__=='__main__':main()
