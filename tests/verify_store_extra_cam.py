#!/usr/bin/env python3
"""Replay three complete paired-target / extra-camera functions on Linux x86-64."""
import ctypes as C
import mmap, pathlib, random, re, struct, subprocess, sys, tempfile
from verify_store_render_release import reader,SPECS,ROOT
# sized constructor, extra init, extra shutdown, target table, extra settings, depth format, device, retry, color helper, image release
RVAS={
'mp':(0x752660,0x753700,0x753840,0x2c3f790,0x2c40090,0x27f2210,0x27f1d88,0x2155dfc,0x752470,0x73b6b0),
'zm':(0x7501a0,0x751240,0x751380,0x2c3d810,0x2c3e110,0x27f0290,0x27efe08,0x2153e7c,0x74ffb0,0x7391e0),
'sp':(0x5f8f80,0x5fa020,0x5fa160,0x27cfa60,0x27ce9b8,0x2381990,0x2381508,0x1cce070,0x5f8d90,0x5de7a0),
}
def relocate(path,read,start,size,offset,translate):
    code=read(start,size);norm=bytearray(code);count=0
    listing=subprocess.check_output(['objdump','-d','-Mintel','--insn-width=16',f'--start-address={0x140000000+start:#x}',f'--stop-address={0x140000000+start+size:#x}',str(path)],text=True)
    for line in listing.splitlines():
        m=re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(.*)',line)
        if not m:continue
        va=int(m[1],16)-0x140000000;raw=bytes.fromhex(m[2]);asm=m[3];end=va-start+len(raw)
        dst=re.search(r'# 0x([0-9a-f]+)',asm) or re.match(r'call\s+0x([0-9a-f]+)',asm)
        if not dst:continue
        target=int(dst[1],16)-0x140000000;slot=translate(target);delta=target-(start+end)
        old=struct.pack('<i',delta);pos=raw.find(old);assert pos>=0 and raw.count(old)==1,(line,old)
        p=va-start+pos;struct.pack_into('<i',code,p,slot-offset-end);norm[p:p+4]=bytes(4);count+=1
    return code,bytes(norm),count

def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'tests/store_render_target_create_shim'),'-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_extra_cam_targets.cpp'),str(ROOT/'tests/store_extra_cam_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.MockExtra.argtypes=[u];lib.MockExtra.restype=p;lib.ExtraCaptureSize.restype=u;lib.RunExtra.argtypes=[p,p]+[u]*8+[p];lib.RunExtra.restype=None
        ref=None
        for mode,(a,b,c,table,state,fmt,device,retry,setup,release) in RVAS.items():
            path=folder/('t6'+mode+'.exe');read=reader(path,SPECS[mode][0]);mem=mmap.mmap(-1,0x5000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
            def translate(t):
                fixed={device:0x1000,retry:0x1008,fmt:0x100c,setup:0x3000,release:0x3020,a:0}
                if t in fixed:return fixed[t]
                if table<=t<table+72*32:return 0x1100+t-table
                if state<=t<state+24:return 0x1020+t-state
                raise ValueError(hex(t))
            norms=[]
            for start,size,offset,expected in [(a,0x117,0,6),(b,0x133,0x200,18),(c,0x1b4,0x400,37)]:
                code,norm,count=relocate(path,read,start,size,offset,translate);assert count==expected,(mode,hex(start),count);mem[offset:offset+len(code)]=code;norms.append(norm)
            if ref is not None:assert ref==norms,mode
            ref=norms;struct.pack_into('<Q',mem,0x1000,lib.MockExtra(0))
            for i,slot in [(1,0x3000),(2,0x3020)]:mem[slot:slot+12]=b'\x48\xb8'+struct.pack('<Q',lib.MockExtra(i))+b'\xff\xe0'
            base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.ExtraCaptureSize());n=C.create_string_buffer(lib.ExtraCaptureSize());rng=random.Random(72);count=0
            for kind,offset in [(0,0),(1,0x200),(2,0x400)]:
                for seed in range(512):
                    for f in (20,40,45,28):
                        dims=[rng.choice((0,1,2,65535,65536,0xffffffff,rng.randrange(2**32))) for _ in range(4)];args=(kind,seed,*dims,f,rng.randrange(9));lib.RunExtra(base+offset,base+0x1000,*args,o);lib.RunExtra(None,base+0x1000,*args,n)
                        assert o.raw==n.raw and struct.unpack_from('<I',o.raw,4)[0]==0,(mode,args,next((i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y),-1));count+=1
            mem.close();print(mode,count,'extra-camera / paired-target comparisons passed',flush=True)
if __name__=='__main__':main()
