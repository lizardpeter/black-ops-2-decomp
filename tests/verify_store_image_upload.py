#!/usr/bin/env python3
"""Replay regular Store image uploads with exact original format-switch tables."""
import ctypes as C
import mmap, pathlib, random, struct, subprocess, sys, tempfile
from verify_store_image_builtin import PE, BUILDS, ROOT
GLOBALS={'mp':(0x41fb970,0x8a72f0,0x27f1d90),'zm':(0x41f99f0,0x8a52f0,0x27efe10),'sp':(0x3f97670,0x7382a8,0x2381510)}
# offset of displacement, instruction end, mock slot, original global index
RELOCS=[(0xce,0xd2,0x1020,0),(0xd9,0xdd,0x1008,1),(0xe0,0xe4,0x1000,2),(0x118,0x11c,0x1020,0),(0x11e,0x122,0x1010,3)]
class Plan(C.Structure):_fields_=[('words',C.c_uint*4)]
class Cap(C.Structure):_fields_=[('plan',Plan),('count',C.c_uint),('bad',C.c_uint),('events',C.c_uint*6)]
def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_upload.cpp'),str(ROOT/'tests/store_image_upload_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));u=C.c_uint;p=C.c_void_p
        lib.UploadPointer.argtypes=[u];lib.UploadPointer.restype=p
        lib.CaptureUpload.argtypes=[p]+[u]*6+[p];lib.CaptureUpload.restype=None
        lib.Store_PlanImageUpload.argtypes=[p]+[u]*6;lib.Store_PlanImageUpload.restype=None
        ref=None
        for mode,spec in BUILDS.items():
            a=spec[-1];pe=PE(folder/('t6'+mode+'.exe'),spec[0]);code=bytearray(pe.read(a,0x1b4));norm=bytearray(code)
            # Original image base LEA and table addresses are left intact in the relocated body.
            assert a+0x66+struct.unpack_from('<i',code,0x62)[0]==0
            norm[0x62:0x66]=bytes(4)
            for i in range(6):
                off=0x150+4*i;struct.pack_into('<I',norm,off,struct.unpack_from('<I',code,off)[0]-a)
            # Normalize the two image-base indexed table operands (verified by target).
            for off,dst in [(0x6a,0x168),(0x71,0x150)]:
                assert struct.unpack_from('<I',code,off)[0]==a+dst
                struct.pack_into('<I',norm,off,dst)
            lock,iat,ctx=GLOBALS[mode];targets=[lock,iat,ctx,iat+8]
            for off,end,slot,idx in RELOCS:
                assert a+end+struct.unpack_from('<i',code,off)[0]==targets[idx],(mode,hex(off))
                struct.pack_into('<i',code,off,slot-end);norm[off:off+4]=bytes(4)
            assert a+0x36+struct.unpack_from('<i',code,0x32)[0]==a-0x350
            struct.pack_into('<i',code,0x32,0x1040-0x36)
            if ref is not None:assert norm==ref,mode
            ref=norm
            mem=mmap.mmap(-1,0x2000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code
            for i in range(3):struct.pack_into('<Q',mem,0x1000+i*8,lib.UploadPointer(i))
            mem[0x1040:0x104c]=b'\x48\xb8'+struct.pack('<Q',lib.UploadPointer(3))+b'\xff\xe0'
            base=C.addressof(C.c_char.from_buffer(mem));rng=random.Random(0xB02);count=0
            for typ in [3,4,5]:
                for fmt in range(100):
                    for mip in [0,1,2,7,15,31,32,63,0xffffffff]:
                        for width in [0,1,3,4,5,65535,rng.randrange(2**32)]:
                            face=rng.randrange(2**32);levels=rng.randrange(2**32);o=Cap();n=Plan()
                            lib.CaptureUpload(base,typ,width,fmt,face,mip,levels,C.byref(o));lib.Store_PlanImageUpload(C.byref(n),typ,width,fmt,face,mip,levels)
                            assert not o.bad and bytes(o.plan)==bytes(n),(mode,typ,fmt,mip,width,list(o.plan.words),list(n.words),o.bad)
                            assert list(o.events)[:o.count]==({0:[],1:[1,2,3,4,5,6],2:[7]}[n.words[0]])
                            count+=1
            mem.close();print(mode,count,'upload plan comparisons passed')
if __name__=='__main__':main()
