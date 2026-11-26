#!/usr/bin/env python3
"""Replay complete program-image allocator including full hash-table behavior."""
import ctypes as C
import mmap,pathlib,struct,subprocess,sys,tempfile
from verify_store_render_release import reader,SPECS,ROOT
RVAS={'mp':(0x73b740,0x91d800,0x444d230,0x4449210),'zm':(0x739270,0x91b800,0x444b2b0,0x4447290),'sp':(0x5de830,0x798e30,0x4085b70,0x4081b50)}
def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        so=pathlib.Path(tmp)/'probe.so'
        subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-DT6_STORE','-I'+str(ROOT/'include'),str(ROOT/'src/gfx_d3d/r_image_prog.cpp'),str(ROOT/'tests/store_image_prog_probe.cpp'),'-o',str(so)],check=True)
        lib=C.CDLL(str(so));lib.ProgCaptureSize.restype=u;lib.RunProg.argtypes=[p,p]+[u]*5+[p];lib.RunProg.restype=None
        ref=None
        for mode,(a,names,images,table) in RVAS.items():
            code=reader(folder/('t6'+mode+'.exe'),SPECS[mode][0])(a,143);norm=bytearray(code)
            for off,end,target,slot in [(6,10,0,0),(0x19,0x1d,images,0x1000)]:
                assert a+end+struct.unpack_from('<i',code,off)[0]==target
                struct.pack_into('<i',code,off,slot-end);norm[off:off+4]=bytes(4)
            for off,target,slot in [(0x12,names,0x1800),(0x5e,table,0x2000),(0x7c,table,0x2000),(0x87,table,0x2000)]:
                assert struct.unpack_from('<I',code,off)[0]==target,(mode,off)
                struct.pack_into('<I',code,off,slot);norm[off:off+4]=bytes(4)
            if ref is not None:assert norm==ref,mode
            ref=norm;mem=mmap.mmap(-1,0x7000,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC);mem[:len(code)]=code;base=C.addressof(C.c_char.from_buffer(mem));o=C.create_string_buffer(lib.ProgCaptureSize());n=C.create_string_buffer(lib.ProgCaptureSize());count=0
            for seed in range(256):
                for fill in (0,1,50,95,99,100):
                    for index in (0,7,15):
                        args=(seed,index,seed&255,(seed*17)&255,fill)
                        lib.RunProg(base,base+0x1000,*args,o);lib.RunProg(None,base+0x1000,*args,n)
                        assert o.raw==n.raw and struct.unpack_from('<I',o.raw)[0]==0,(mode,args)
                        count+=1
            mem.close();print(mode,count,'complete allocator comparisons passed',flush=True)
if __name__=='__main__':main()
