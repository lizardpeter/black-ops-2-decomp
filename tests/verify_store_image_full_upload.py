#!/usr/bin/env python3
"""Replay complete Store upload bodies with mocked COM and successful maps."""
import ctypes as C
import hashlib, mmap, pathlib, struct, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
SPECS={
 'mp':('cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01',0x7770f0,0x41fb970,0x8a72f0,0x27f1d90,0x870aa2),
 'zm':('7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416',0x774c30,0x41f99f0,0x8a52f0,0x27efe10,0x86e592),
 'sp':('0404f79cb36394b14cfc3321b7aa126a08518531564fa042fc14b82debe45c89',0x61c9e0,0x3f97670,0x7382a8,0x2381510,0x703962),
}
def read_pe(path,digest,rva,size):
    b=path.read_bytes();assert hashlib.sha256(b).hexdigest()==digest,path
    h,=struct.unpack_from('<I',b,60);n,=struct.unpack_from('<H',b,h+6);opt,=struct.unpack_from('<H',b,h+20)
    for i in range(n):
        vs,va,rs,raw=struct.unpack_from('<4I',b,h+24+opt+40*i+8)
        if va<=rva and rva+size<=va+rs:return b[raw+rva-va:raw+rva-va+size]
    raise ValueError(hex(rva))
def main():
    folder=pathlib.Path(sys.argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_image_upload_shim'),str(ROOT/'src/gfx_d3d/r_image_upload_native.cpp'),str(ROOT/'tests/store_image_full_upload_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.UploadMock.argtypes=[C.c_uint];lib.UploadMock.restype=C.c_void_p;lib.CaptureSize.restype=C.c_uint;lib.CaptureFull.argtypes=[C.c_void_p]+[C.c_uint]*9+[C.c_void_p];lib.CaptureFull.restype=None
        for mode,(digest,a,lock,iat,ctx,copy) in SPECS.items():
            code=bytearray(read_pe(folder/('t6'+mode+'.exe'),digest,a,0x504))
            rel=[(0xe8,0xec,lock,0x1020),(0xee,0xf2,iat,0x1008),(0x122,0x126,ctx,0x1000),(0x1d6,0x1da,copy,0x1040),(0x1fa,0x1fe,copy,0x1040),(0x22b,0x22f,ctx,0x1000),(0x269,0x26d,lock,0x1020),(0x26f,0x273,iat+8,0x1010)]
            rel += [(0x350+x,0x350+y,t,s) for x,y,t,s in [(0xce,0xd2,lock,0x1020),(0xd9,0xdd,iat,0x1008),(0xe0,0xe4,ctx,0x1000),(0x118,0x11c,lock,0x1020),(0x11e,0x122,iat+8,0x1010)]]
            for off,end,target,slot in rel:
                assert a+end+struct.unpack_from('<i',code,off)[0]==target,(mode,hex(off))
                struct.pack_into('<i',code,off,slot-end)
            mem=mmap.mmap(-1,0x2000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code
            for i in range(3):struct.pack_into('<Q',mem,0x1000+8*i,lib.UploadMock(i))
            mem[0x1040:0x104c]=b'\x48\xb8'+struct.pack('<Q',lib.UploadMock(3))+b'\xff\xe0'
            base=C.addressof(C.c_char.from_buffer(mem));size=lib.CaptureSize();o=C.create_string_buffer(size);n=C.create_string_buffer(size);count=0
            for typ in (3,4,5):
                for fmt in range(100):
                    for w,h,d in ((1,1,1),(3,5,3),(8,9,4),(16,16,8),(0,0,0)):
                        for mip in (0,1,2,31,32):
                            mw=max(1,w>>(mip&31));tight={10:mw*4,11:mw*4,28:mw*4,35:mw*4,41:mw*4,65:mw,71:((mw+3)//4)*8,74:((mw+3)//4)*16,77:((mw+3)//4)*16,85:mw*2}.get(fmt,1)
                            for pitch in (tight,tight+13):
                                args=(typ,w,h,d,fmt,5,mip,7,pitch)
                                lib.CaptureFull(base+0x350,*args,o);lib.CaptureFull(None,*args,n)
                                assert o.raw==n.raw,(mode,args,next(i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y))
                                assert struct.unpack_from('<I',n.raw,4)[0]==0
                                count+=1
            mem.close();print(mode,count,'complete upload comparisons passed',flush=True)
if __name__=='__main__':main()
