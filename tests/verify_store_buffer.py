#!/usr/bin/env python3
"""Replay complete static and dynamic vertex/index buffer lifecycle."""
import ctypes as C
import mmap,pathlib,struct,subprocess,sys,tempfile
from verify_store_render_release import reader,SPECS,ROOT
from verify_store_extra_cam import relocate
# first constructor, dvar, device, retry, atoi IAT, skinned pool, allocation, fatal, VirtualFree IAT
RVAS={
'mp':(0x738a80,0x27e7070,0x27f1d88,0x2155dfc,0x8a7dc0,0x2c3e730,0x465270,0x49b3f0,0x8a72c8),
'zm':(0x736630,0x27e50f0,0x27efe08,0x2153e7c,0x8a5dc0,0x2c3c7b0,0x465f60,0x49c060,0x8a52c8),
'sp':(0x5df580,0x2369c18,0x2381508,0x1cce070,0x738d10,0x27cf020,0x3f4a20,0x40b390,0x738280),
}
def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_buffer_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_buffer_store.cpp'),str(ROOT/'tests/store_buffer_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.MockBuffer.argtypes=[u];lib.MockBuffer.restype=p;lib.BufferCaptureSize.restype=u;lib.RunBuffer.argtypes=[p,p]+[u]*7+[p];lib.RunBuffer.restype=None
        ref=None
        for mode,(a,dvar,dev,retry,parse,pool,alloc,fatal,free) in RVAS.items():
            path=folder/('t6'+mode+'.exe');read=reader(path,SPECS[mode][0]);mem=mmap.mmap(-1,0x4000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
            def translate(t):
                fixed={dev:0x1000,retry:0x1008,dvar:0x1010,parse:0x1018,free:0x1020,alloc:0x2000,fatal:0x2020,a+0x180:0x180}
                if t in fixed:return fixed[t]
                if pool<=t<pool+48:return 0x1100+t-pool
                raise ValueError(hex(t))
            specs=[(0,0xa3,4),(0xb0,0xc8,4),(0x180,0xa5,4),(0x230,0x7c,5),(0x2b0,0x6a,3)]
            norms=[]
            for off,size,count in specs:
                code,norm,n=relocate(path,read,a+off,size,off,translate);assert n==count,(mode,hex(off),n);mem[off:off+len(code)]=code;norms.append(norm)
            if ref is not None:assert ref==norms,mode
            ref=norms
            for i,slot in [(0,0x1000),(1,0x1018),(4,0x1020)]:struct.pack_into('<Q',mem,slot,lib.MockBuffer(i))
            for i,slot in [(2,0x2000),(3,0x2020)]:mem[slot:slot+12]=b'\x48\xb8'+struct.pack('<Q',lib.MockBuffer(i))+b'\xff\xe0'
            base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.BufferCaptureSize());n=C.create_string_buffer(lib.BufferCaptureSize());count=0
            for kind,(off,_,__) in enumerate(specs):
                for val in (0,1,16,65535,65536,0x80000000,0xffffffff):
                    for cfg in range(5):
                        for initial in (0,1):
                            for repeats in (1,2,3):
                                for fail in ((0,1,2) if kind==3 else (0,)):
                                    for pattern in range(16):
                                        args=(kind,val,cfg,initial,repeats,fail,pattern);lib.RunBuffer(base+off,base+0x1000,*args,o);lib.RunBuffer(None,base+0x1000,*args,n)
                                        assert o.raw==n.raw and struct.unpack_from('<I',o.raw,4)[0]==0,(mode,args,next((i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y),-1));count+=1
            mem.close();print(mode,count,'complete buffer comparisons passed',flush=True)
if __name__=='__main__':main()
