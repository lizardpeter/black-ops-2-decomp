#!/usr/bin/env python3
"""Replay complete Store texture constructors and setup with mocked COM and accounting."""
import ctypes as C
import hashlib, mmap, pathlib, random, struct, subprocess, sys, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
SPECS={
 'mp':('cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01',0x73b7d0,0x7776a0,0x282e538,0x27f1d88,0x2155dfc,0x27e7070,0x8a7dc0),
 'zm':('7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416',0x739300,0x7751e0,0x282c5b8,0x27efe08,0x2153e7c,0x27e50f0,0x8a5dc0),
 'sp':('0404f79cb36394b14cfc3321b7aa126a08518531564fa042fc14b82debe45c89',0x5de8c0,0x61cf90,0x23bdcb8,0x2381508,0x1cce070,0x2369c18,0x738d10),
}
def read_pe(path,digest):
    b=path.read_bytes();assert hashlib.sha256(b).hexdigest()==digest,path
    h,=struct.unpack_from('<I',b,60);n,=struct.unpack_from('<H',b,h+6);opt,=struct.unpack_from('<H',b,h+20)
    sections=[struct.unpack_from('<4I',b,h+24+opt+40*i+8) for i in range(n)]
    def read(a,size):
        for vs,va,rs,raw in sections:
            if va<=a and a+size<=va+rs:return bytearray(b[raw+a-va:raw+a-va+size])
        raise ValueError(hex(a))
    return read

def main():
    folder=pathlib.Path(sys.argv[1]);u=C.c_uint;p=C.c_void_p
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_image_create_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_create.cpp'),str(ROOT/'src/gfx_d3d/r_image_descriptor.cpp'),str(ROOT/'tests/store_image_create_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.Mock.argtypes=[u];lib.Mock.restype=p;lib.CaptureSize.restype=u;lib.CaptureCreate.argtypes=[p,p]+[u]*13+[p];lib.CaptureCreate.restype=None
        ref=None
        for mode,(digest,a,s,ms,device,retry,dvar,atoi) in SPECS.items():
            read=read_pe(folder/('t6'+mode+'.exe'),digest);code=read(a,0x3c8);setup=read(s,0x11a);norm=bytearray(code);ns=bytearray(setup)
            rel=[(0x29,0x2e,ms,0),(0xa3,0xa7,ms,0),(0xac,0xb0,ms+4,4),(0xf4,0xf8,device,8),(0x10c,0x110,retry,16),(0x173,0x177,device,8),(0x18b,0x18f,retry,16),(0x243,0x247,device,8),(0x25c,0x261,retry,16),(0x284,0x288,device,8),(0x29d,0x2a2,retry,16),(0x343,0x347,device,8),(0x35f,0x363,retry,16),(0x386,0x38a,device,8),(0x3a2,0x3a6,retry,16)]
            for off,end,target,slot in rel:
                assert a+end+struct.unpack_from('<i',code,off)[0]==target,(mode,hex(off));struct.pack_into('<i',code,off,0x1000+slot-end);norm[off:off+4]=bytes(4)
            for off,end,target,slot in [(0x42,0x46,dvar,0x1018),(0x64,0x68,atoi,0x1020),(0xa2,0xa6,a+0x2d0,0x2d0),(0xc6,0xca,a+0x1c0,0x1c0),(0xdd,0xe1,a,0),(0xf7,0xfb,s-0x800,0x1040)]:
                assert s+end+struct.unpack_from('<i',setup,off)[0]==target,(mode,hex(off));struct.pack_into('<i',setup,off,slot-0x600-end);ns[off:off+4]=bytes(4)
            if ref is not None:assert ref==(norm,ns),mode
            ref=norm,ns
            mem=mmap.mmap(-1,0x2000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code;mem[0x600:0x600+len(setup)]=setup
            struct.pack_into('<Q',mem,0x1008,lib.Mock(0));struct.pack_into('<Q',mem,0x1020,lib.Mock(1));mem[0x1040:0x104c]=b'\x48\xb8'+struct.pack('<Q',lib.Mock(2))+b'\xff\xe0'
            base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.CaptureSize());n=C.create_string_buffer(lib.CaptureSize());rng=random.Random(618);count=0
            for kind,entry in [(3,0),(4,0x1c0),(5,0x2d0),(0,0x600)]:
                for fmt in range(100):
                    for flags in (0,2,4,6,8,10,12,0x20000,0x40000,0x60000):
                        for config in (0,1,2,3,4):
                            w,h,d,levels,quality=[rng.randrange(100000) for _ in range(5)];samples=rng.choice((0,1,2,4,8));cr=rng.randrange(1,4);vr=rng.randrange(1,4);use=rng.randrange(2)
                            args=kind,w,h,d,levels,flags,fmt,samples,quality,config,cr,vr,use
                            lib.CaptureCreate(base+entry,base+0x1000,*args,o);lib.CaptureCreate(None,base+0x1000,*args,n)
                            assert o.raw==n.raw,(mode,args,next(i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y))
                            assert struct.unpack_from('<I',o.raw,4)[0]==0
                            count+=1
            mem.close();print(mode,count,'complete constructor/setup comparisons passed',flush=True)
if __name__=='__main__':main()
