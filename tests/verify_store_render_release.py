#!/usr/bin/env python3
"""Replay original image release and target shutdown with capture-only COM objects."""
import ctypes as C
import hashlib,mmap,pathlib,struct,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
SPECS={
'mp':('cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01',0x73b6b0,0x753660,0x444d21c,0x2c64194,0x27e7070,0x8a7dc0,0x2c3f790,0x444dbf0,0x444d6a8,0x444e138,0x282e548),
'zm':('7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416',0x7391e0,0x7511a0,0x444b29c,0x2c62214,0x27e50f0,0x8a5dc0,0x2c3d810,0x444bc70,0x444b728,0x444c1b8,0x282c5c8),
'sp':('0404f79cb36394b14cfc3321b7aa126a08518531564fa042fc14b82debe45c89',0x5de7a0,0x5f9f80,0x4085b5c,0x27f4214,0x2369c18,0x738d10,0x27cfa60,0x4086530,0x4085fe8,0x4086a78,0x23bdcc8),
}
def reader(path,digest):
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
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_render_release_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_release.cpp'),str(ROOT/'tests/store_render_release_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.ParseMock.restype=p;lib.CaptureSize.restype=u;lib.Run.argtypes=[p,p]+[u]*6+[p];lib.Run.restype=None
        ref=None
        for mode,(digest,a,b,total,loaded,dvar,parse,targets,x,y,z,state) in SPECS.items():
            read=reader(folder/('t6'+mode+'.exe'),digest);one=read(a,134);two=read(b,152);norm=[]
            for code,start,offset,rel in [(one,a,0,[(0x20,0x24,total,0x1000),(0x29,0x2d,total+4,0x1004),(0x53,0x57,dvar,0x1010),(0x68,0x6c,parse,0x1018),(0x71,0x75,loaded,0x1008)]),(two,b,0x100,[(0xd,0x11,targets,0x1100),(0x14,0x18,targets+0x900,0x1a00),(0x47,0x4b,a,0),(0x61,0x65,x,0x1b00),(0x66,0x6a,a,0),(0x6d,0x71,y,0x1c00),(0x72,0x76,a,0),(0x79,0x7d,z,0x1d00),(0x7e,0x82,a,0),(0x8a,0x92,state,0x1020)])]:
                original=bytearray(code)
                for off,end,target,slot in rel:
                    assert start+end+struct.unpack_from('<i',code,off)[0]==target,(mode,hex(start),hex(off),hex(start+end+struct.unpack_from('<i',code,off)[0]),hex(target))
                    struct.pack_into('<i',code,off,slot-offset-end);original[off:off+4]=bytes(4)
                norm.append(original)
            if ref is not None:assert ref==norm,mode
            ref=norm;mem=mmap.mmap(-1,0x3000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(one)]=one;mem[0x100:0x100+len(two)]=two;struct.pack_into('<Q',mem,0x1018,lib.ParseMock());base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.CaptureSize());n=C.create_string_buffer(lib.CaptureSize());count=0
            for track in range(256):
                for view in (0,1):
                    for delay in (0,1):
                        for mode_dvar in range(5):
                            args=(track*17+view,0,track,view,delay,mode_dvar);lib.Run(base,base+0x1000,*args,o);lib.Run(None,base+0x1000,*args,n)
                            assert o.raw==n.raw,(mode,args,next(i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y));count+=1
            for seed in range(256):
                for mode_dvar in range(5):
                    args=(seed,1,0,0,0,mode_dvar);lib.Run(base+0x100,base+0x1000,*args,o);lib.Run(None,base+0x1000,*args,n)
                    assert o.raw==n.raw,(mode,args,next(i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y));count+=1
            mem.close();print(mode,count,'complete release/shutdown comparisons passed',flush=True)
if __name__=='__main__':main()
