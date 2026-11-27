#!/usr/bin/env python3
"""Replay complete UI_RegisterDvars bodies with mocked Store registration callees."""
import ctypes as C
import hashlib, mmap, pathlib, struct, subprocess, sys, tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent / 'replay'))
import replay
ROOT=pathlib.Path(__file__).resolve().parents[1]
SPECS={
'mp':('cf80b8058bce7e00e4c61ff3d673308c1fcd8061f18fe66710803538eab4bc01',0x3f9b20,0x3fa389,[0x46b8b0,0x46c1a0,0x46bf10,0x46c650,0x46ca30,0x46cbd0,0x46c4c0,0x404a20,0x408bc0],[0x2121490,0x2126538,0x2155cf8,0x2121410,0x2121418,0x2121420,0x2121428,0x2121430,0x2121448,0x2121368,0x21213a8,0x2121398,0x2155d30,0x2121338,0x2121328,0x2121350,0x21214a8,0x2121320,0x2155d20,0x2121318,0x2155d08,0x2155d10,0x2155d18,0x2155d28,0x2121330,0x2126530,0x2121360,0x2121438,0x2121380,0x21214b0,0x2126548,0x2121408,0x21214a0,0x2121370,0x2121388,0x2126540,0x2121358,0x2121340,0x2121348,0x2121498,0x2126550,0x21213b0,0x2155cc8,0x2155cc0,0x2155cb8,0x21213a0,0x2121440,0x2121378,0x2121400,0x2121390,0x21214b8,0x21212f0,0x21212f8]),
'zm':('7152ebe1c892ea8b1ea7a73811cb82bcdc22088816f5c731a6637d5461bb7416',0x3fa4f0,0x3fad59,[0x46c5a0,0x46ce90,0x46cc00,0x46d340,0x46d720,0x46d8c0,0x46d1b0,0x405550,0x409e20],[0x211f510,0x21245b8,0x2153d78,0x211f490,0x211f498,0x211f4a0,0x211f4a8,0x211f4b0,0x211f4c8,0x211f3e8,0x211f428,0x211f418,0x2153db0,0x211f3b8,0x211f3a8,0x211f3d0,0x211f528,0x211f3a0,0x2153da0,0x211f398,0x2153d88,0x2153d90,0x2153d98,0x2153da8,0x211f3b0,0x21245b0,0x211f3e0,0x211f4b8,0x211f400,0x211f530,0x21245c8,0x211f488,0x211f520,0x211f3f0,0x211f408,0x21245c0,0x211f3d8,0x211f3c0,0x211f3c8,0x211f518,0x21245d0,0x211f430,0x2153d48,0x2153d40,0x2153d38,0x211f420,0x211f4c0,0x211f3f8,0x211f480,0x211f410,0x211f538,0x211f370,0x211f378]),
}
def image(path,digest):
    data=path.read_bytes();assert hashlib.sha256(data).hexdigest()==digest,path
    nt,=struct.unpack_from('<I',data,60);count,=struct.unpack_from('<H',data,nt+6);opt,=struct.unpack_from('<H',data,nt+20);size,=struct.unpack_from('<I',data,nt+0x50)
    mem=mmap.mmap(-1,size,prot=mmap.PROT_READ|mmap.PROT_WRITE|mmap.PROT_EXEC)
    for i in range(count):
        vs,va,rs,raw=struct.unpack_from('<4I',data,nt+24+opt+40*i+8);mem[va:va+rs]=data[raw:raw+rs]
    return mem

def main():
    folder=pathlib.Path(sys.argv[1]);p=C.c_void_p;u=C.c_uint
    with tempfile.TemporaryDirectory() as tmp:
        lib=replay.build('../store_ui_registration_probe.cpp',tmp,['ui/ui_register_dvars.cpp','universal/dvar.cpp'])
        lib.UiRegistrationMock.argtypes=[u];lib.UiRegistrationMock.restype=p;lib.UiRegistrationRun.argtypes=[p,u,C.c_bool,p];lib.UiRegistrationRun.restype=None;lib.UiRegistrationCaptureSize.restype=u;lib.UiRegistrationGlobals.argtypes=[p];lib.UiRegistrationGlobals.restype=u
        for mode,(digest,entry,default,targets,globals_) in SPECS.items():
            mem=image(folder/('t6'+mode+'.exe'),digest);base=C.addressof(C.c_char.from_buffer(mem));assert mem[default-3:default+1]==b'\xc6\x45\xd7\x01'
            for i,t in enumerate(targets):mem[t:t+12]=b'\x48\xb8'+struct.pack('<Q',lib.UiRegistrationMock(i))+b'\xff\xe0'
            o=C.create_string_buffer(lib.UiRegistrationCaptureSize());n=C.create_string_buffer(lib.UiRegistrationCaptureSize());g=(u*53)();cases=0
            for draw in (True,False):
                mem[default]=int(draw)
                for seed in range(128):
                    for rva in globals_:struct.pack_into('<Q',mem,rva,0)
                    lib.UiRegistrationRun(base+entry,seed,draw,o);lib.UiRegistrationRun(None,seed,draw,n)
                    assert o.raw==n.raw,(mode,draw,seed,next((i for i,(x,y) in enumerate(zip(o.raw,n.raw)) if x!=y),-1))
                    assert struct.unpack_from('<II',o.raw)[1]==0,(mode,draw,seed,struct.unpack_from('<II',o.raw))
                    assert lib.UiRegistrationGlobals(g)==53
                    assert list(g)==[struct.unpack_from('<Q',mem,rva)[0] for rva in globals_],(mode,draw,seed)
                    cases+=1
            mem.close();print(mode,cases,'complete registration comparisons; identical ordered events and 53 global pointers per case',flush=True)
if __name__=='__main__':main()
