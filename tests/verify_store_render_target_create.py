#!/usr/bin/env python3
"""Replay complete Store render-target setup, with captured allocation and COM calls."""
import ctypes as C
import mmap,pathlib,random,struct,subprocess,sys,tempfile
from verify_store_render_release import reader,SPECS,ROOT
RVAS={
'mp':(0x752470,0x73b740,0x7776a0,0x9b0398,0x9b0358,0x9b04e0,0x890c7f,0x27f1d88,0x282e538,0x2155dfc),
'zm':(0x74ffb0,0x739270,0x7751e0,0x9ae178,0x9ae138,0x9ae2c0,0x88e76f,0x27efe08,0x282c5b8,0x2153e7c),
'sp':(0x5f8d90,0x5de830,0x61cf90,0x814fa8,0x815108,0x8150d0,0x723a2f,0x2381508,0x23bdcb8,0x1cce070),
}
def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_render_target_create_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_render_target_create.cpp'),str(ROOT/'tests/store_render_target_create_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.MockTarget.argtypes=[u];lib.MockTarget.restype=p;lib.TargetCaptureSize.restype=u;lib.CaptureTarget.argtypes=[p,p]+[u]*8+[p];lib.CaptureTarget.restype=None
        ref=None
        for mode,(a,alloc,setup,scene,ssao,dof,cmp,device,ms,retry) in RVAS.items():
            read=reader(folder/('t6'+mode+'.exe'),SPECS[mode][0]);code=read(a,493);norm=bytearray(code);special=SPECS[mode][7]+0x80
            for off,end,target,slot in [(0x2d,0x31,alloc,0x2000),(0x3c,0x40,scene,0x1200),(0x7d,0x81,ssao,0x1220),(0x85,0x89,cmp,0x2040),(0xa5,0xa9,dof,0x1240),(0xad,0xb1,cmp,0x2040),(0xe2,0xe6,setup,0x2020),(0x11d,0x121,device,0x1000),(0x13d,0x141,special,0x3080),(0x148,0x14d,ms,0x1008),(0x193,0x197,device,0x1000),(0x1b0,0x1b5,retry,0x1010)]:
                actual=a+end+struct.unpack_from('<i',code,off)[0];assert actual==target,(mode,hex(off),hex(actual),hex(target));struct.pack_into('<i',code,off,slot-end);norm[off:off+4]=bytes(4)
            if ref is not None:assert ref==norm,mode
            ref=norm;mem=mmap.mmap(-1,0x5000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code;struct.pack_into('<Q',mem,0x1000,lib.MockTarget(0))
            for i,slot in enumerate((0x2000,0x2020,0x2040),1):mem[slot:slot+12]=b'\x48\xb8'+struct.pack('<Q',lib.MockTarget(i))+b'\xff\xe0'
            for slot,rva,s in [(0x1200,scene,b'$scene\0'),(0x1220,ssao,b'$ssao_csz\0'),(0x1240,dof,b'$dof_scene_fp16\0')]:assert read(rva,len(s))==s;mem[slot:slot+len(s)]=s
            base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.TargetCaptureSize());n=C.create_string_buffer(lib.TargetCaptureSize());rng=random.Random(0x752470);count=0
            for name in range(8):
                for fmt in range(100):
                    for color in (0,1):
                        for special in (0,1):
                            for samples in (0,1,2,8):
                                args=(name,rng.randrange(2**32),rng.randrange(2**32),fmt,color,special,samples,rng.randrange(1,4));lib.CaptureTarget(base,base+0x1000,*args,o);lib.CaptureTarget(None,base+0x1000,*args,n)
                                assert o.raw==n.raw and struct.unpack_from('<I',o.raw,4)[0]==0,(mode,args,next((i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y),-1));count+=1
            mem.close();print(mode,count,'complete render-target setup comparisons passed',flush=True)
if __name__=='__main__':main()
