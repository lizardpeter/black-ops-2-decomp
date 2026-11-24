#!/usr/bin/env python3
"""Original public XCurl getinfo + profiling + lock boundaries (disabled ETW)."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,OBJECTS
from differential_xcurl_getinfo import SHA256,OPTIONS
from unicorn import x86_const as X
class Profile(C.Structure):
    _fields_=[("first_tick",C.c_uint64),("counts",C.c_uint32*32)]
LOCK=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
TICK=C.CFUNCTYPE(C.c_uint64,C.c_void_p)
ERROR=C.CFUNCTYPE(None,C.c_uint32,C.c_uint32,C.c_void_p)
A=0x34000000
OUT=A+0x1000
def main():
    dll,libpath=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000)
    lib=C.CDLL(str(libpath.resolve()))
    fn=lib.xcurl_research_easy_getinfo
    fn.argtypes=(C.c_void_p,C.c_int,C.c_void_p,C.POINTER(Profile),C.c_void_p,LOCK,LOCK,TICK,ERROR,C.c_void_p)
    fn.restype=C.c_uint32
    rng=random.Random(0xdfc0)
    for i in range(512):
        h=rng.randbytes(0x240);filler=rng.randbytes(16)
        opt=OPTIONS[i%len(OPTIONS)]
        present=i%8!=0
        first=0 if i%3==0 else rng.getrandbits(64)
        counts=[rng.getrandbits(32) for _ in range(32)]
        if i%4==0:counts[10]=0xffffffff
        clock=rng.getrandbits(64)
        oc=[];nc=[]
        def olock(mu,phase):
            address=mu.reg_read(X.UC_X86_REG_RCX)
            assert address in (img.base+0x28b58,A+0x188)
            oc.append((phase,"global" if address==img.base+0x28b58 else "easy"))
        def otick(mu):oc.append(("tick",));mu.reg_write(X.UC_X86_REG_RAX,clock)
        def oerror(mu):
            oc.append(("error",mu.reg_read(X.UC_X86_REG_ECX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
        for name in img.iat.values():
            if name.endswith("!EnterCriticalSection"):m.on_import(name,lambda mu:olock(mu,"enter"))
            if name.endswith("!LeaveCriticalSection"):m.on_import(name,lambda mu:olock(mu,"leave"))
            if name.endswith("!GetTickCount64"):m.on_import(name,otick)
        def setup(w):
            w.write(img.base+0x28ba0,struct.pack("<Q",first))
            w.write(img.base+0x28bb0,struct.pack("<32I",*counts))
            m.mu.mem_write(A,h);m.mu.mem_write(OUT,filler)
        ret,_,writes,_=m.call(img.exports["curl_easy_getinfo"],[A if present else 0,opt,OUT],"uint",
                               setup=setup,fakes={img.base+0x83d8:oerror})
        assert isinstance(ret,int),(i,ret,m.imports_called)
        def updated(a,old):return b"".join(writes.get(img.base+a+j,bytes([v])) for j,v in enumerate(old))
        expected_first=struct.unpack("<Q",updated(0x28ba0,struct.pack("<Q",first)))[0]
        expected_counts=struct.unpack("<32I",updated(0x28bb0,struct.pack("<32I",*counts)))
        native_h=C.create_string_buffer(h,len(h));native_out=C.create_string_buffer(filler,len(filler))
        prof=Profile(first,(C.c_uint32*32)(*counts))
        sentinel=0x123456
        def nlock(p,phase):
            assert p in (sentinel,C.addressof(native_h)+0x188)
            nc.append((phase,"global" if p==sentinel else "easy"))
        @LOCK
        def en(p,ctx):nlock(p,"enter")
        @LOCK
        def le(p,ctx):nlock(p,"leave")
        @TICK
        def tick(ctx):nc.append(("tick",));return clock
        @ERROR
        def log(code,api,ctx):nc.append(("error",code,api))
        got=fn(native_h if present else None,opt,native_out,C.byref(prof),sentinel,en,le,tick,log,None)
        assert got==ret and oc==nc,(i,opt,got,ret,oc,nc)
        assert prof.first_tick==expected_first and tuple(prof.counts)==expected_counts,(i,expected_first,prof.first_tick,expected_counts,tuple(prof.counts))
        assert bytes(native_out)==bytes(m.mu.mem_read(OUT,16))
        assert bytes(native_h)==h and bytes(m.mu.mem_read(A,len(h)))==h
    print("XCurl public getinfo/profile/lock/error boundary PASS: 512 cases")
if __name__=="__main__":main()
