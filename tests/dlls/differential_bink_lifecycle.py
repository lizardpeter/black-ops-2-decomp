#!/usr/bin/env python3
"""Compare original export lifecycle transitions; queue/thread boundaries injected."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine
from differential_bink_resident_state import PIN
from unicorn import x86_const as X
class State(C.Structure):
    _fields_=[("running",C.c_uint32),("stop_requested",C.c_uint32),("error_rva",C.c_uint32)]
INIT=C.CFUNCTYPE(C.c_int,C.c_uint32,C.c_uint,C.c_void_p)
DEST=C.CFUNCTYPE(None,C.c_uint32,C.c_uint,C.c_void_p)
LAUNCH=C.CFUNCTYPE(C.c_int,C.c_uint32,C.c_void_p)
STOP=C.CFUNCTYPE(C.c_int,C.c_uint32,C.c_size_t,C.c_void_p)
def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img);lib=C.CDLL(str(source.resolve()))
    start=lib.bink_research_start_worker;start.argtypes=(C.POINTER(State),C.c_uint32,INIT,DEST,LAUNCH,C.c_void_p)
    stop=lib.bink_research_stop_worker;stop.argtypes=(C.POINTER(State),C.c_uint32,STOP,C.c_void_p)
    join=lib.bink_research_join_worker;join.argtypes=(C.POINTER(State),C.c_uint32,LAUNCH,DEST,C.c_void_p)
    rng=random.Random(0x21de0)
    for action,fn in (("Start",start),("RequestStop",stop),("WaitStop",join)):
        for i in range(320):
            n=rng.randrange(10);running=rng.randrange(256);stopped=rng.randrange(256)
            outcomes=[rng.randrange(2) for _ in range(3)]
            olderr=img.base+0x5f550
            original=[];native=[]
            def queue(mu):
                addr=mu.reg_read(X.UC_X86_REG_RCX)-img.base
                q=0 if addr==0x321e0+n*0x14d0 else 1
                assert addr in (0x321e0+n*0x14d0,0x3c860+n*0x14d0)
                original.append(("init",q));return outcomes[q]
            def destroy(mu):
                addr=mu.reg_read(X.UC_X86_REG_RCX)-img.base
                q=0 if addr==0x321e0+n*0x14d0 else 1
                assert addr in (0x321e0+n*0x14d0,0x3c860+n*0x14d0)
                original.append(("destroy",q));return 0
            def launch(mu):
                assert mu.reg_read(X.UC_X86_REG_RCX)==img.base+0x31960+n*0x110
                assert mu.reg_read(X.UC_X86_REG_EDX)==n
                original.append(("launch",));return outcomes[2]
            def request(mu):
                assert mu.reg_read(X.UC_X86_REG_RCX)==img.base+0x321e0+n*0x14d0
                assert mu.reg_read(X.UC_X86_REG_R8D)==8
                assert bytes(mu.mem_read(mu.reg_read(X.UC_X86_REG_RDX),8))==bytes(8)
                original.append(("stop",));return outcomes[0]
            def wait(mu):
                assert mu.reg_read(X.UC_X86_REG_RCX)==img.base+0x31960+n*0x110
                original.append(("join",));return outcomes[0]
            def setup(w):
                for a,v,fmt in [(0x570a4,running,"I"),(0x570bc,stopped,"I"),(0x57028,olderr,"Q")]:
                    w.write(img.base+a,struct.pack("<"+fmt,v))
            ret,_,writes,_=m.call(img.exports["Bink"+action+"AsyncThread"],[n,0],"uint",setup=setup,
                fakes={img.base+r:cb for r,cb in [(0x21820,queue),(0x21900,destroy),(0x219e0,launch),(0x21aa0,request),(0x21d60,wait)]})
            def after(rva,old,fmt):
                b=struct.pack("<"+fmt,old)
                return struct.unpack("<"+fmt,b"".join(writes.get(img.base+rva+k,b[k:k+1]) for k in range(len(b))))[0]
            state=State(running,stopped,0x5f550)
            @INIT
            def ni(worker,q,ctx):native.append(("init",q));return outcomes[q]
            @DEST
            def nd(worker,q,ctx):native.append(("destroy",q))
            @LAUNCH
            def nl(worker,ctx):native.append(("launch",));return outcomes[2]
            @STOP
            def ns(worker,msg,ctx):native.append(("stop",));assert msg==0;return outcomes[0]
            @LAUNCH
            def nw(worker,ctx):native.append(("join",));return outcomes[0]
            args=(ni,nd,nl) if action=="Start" else (ns,) if action=="RequestStop" else (nw,nd)
            got=fn(C.byref(state),n,*args,None)
            err=after(0x57028,olderr,"Q")
            expected=(after(0x570a4,running,"I"),after(0x570bc,stopped,"I"),err-img.base if err else 0)
            assert ret==got and original==native and expected==(state.running,state.stop_requested,state.error_rva),(action,i,n,ret,got,original,native,expected,(state.running,state.stop_requested,state.error_rva))
            assert not m.imports_called
        print("Original "+action+"AsyncThread PASS: 320 state/queue cases")
if __name__=="__main__":main()
