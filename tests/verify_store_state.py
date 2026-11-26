#!/usr/bin/env python3
"""Replay complete original render-state functions against authored code."""
import ctypes as C
import mmap,pathlib,struct,subprocess,sys,tempfile
from verify_store_render_release import reader,SPECS,ROOT
from verify_store_extra_cam import relocate
RVAS={'mp':(0x764f20,0x766f00,0x91e100,0x2c3f790,0x282e5f8),'zm':(0x762a60,0x764a40,0x91c100,0x2c3d810,0x282c678),'sp':(0x60ac80,0x60cd20,0x79dcb0,0x27cfa60,0x23bdd78)}
def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_state_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_state_store.cpp'),str(ROOT/'tests/store_state_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.StateCaptureSize.restype=u;lib.RunState.argtypes=[p,p]+[u]*6+[p];lib.RunState.restype=None
        ref=None
        for mode,(a,b,matrix,table,count) in RVAS.items():
            path=folder/('t6'+mode+'.exe');read=reader(path,SPECS[mode][0]);mem=mmap.mmap(-1,0x3000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
            def translate(t):
                if t==table:return 0x1100
                if t==count:return 0x1000
                if matrix<=t<matrix+64:return 0x800+t-matrix
                raise ValueError(hex(t))
            norms=[]
            for start,size,off,expected in [(a,156,0,4),(a+0xa0,248,0xa0,0),(b,482,0x200,3),(b+0x1f0,165,0x400,1)]:
                code,norm,n=relocate(path,read,start,size,off,translate);assert n==expected,(mode,start,n);mem[off:off+len(code)]=code;norms.append(norm)
            if ref is not None:assert ref==norms,mode
            ref=norms;mem[0x800:0x840]=read(matrix,64);assert struct.unpack('<16f',mem[0x800:0x840])==(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
            base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.StateCaptureSize());n=C.create_string_buffer(lib.StateCaptureSize());cases=0
            for kind,off in enumerate((0,0xa0,0x200,0x400)):
                for seed in range(128):
                    for mode_arg in (0,1,2,3,7,11):
                        for pattern in (0,1,17,35,70,127):
                            args=(kind,seed,mode_arg,seed&255,pattern,seed%17)
                            lib.RunState(base+off,base+0x1000,*args,o);lib.RunState(None,base+0x1000,*args,n)
                            assert o.raw==n.raw and struct.unpack_from('<I',o.raw,4)[0]==0,(mode,args,next((i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y),-1));cases+=1
            mem.close();print(mode,cases,'complete state comparisons passed',flush=True)
if __name__=='__main__':main()
