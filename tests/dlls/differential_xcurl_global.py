#!/usr/bin/env python3
"""Compare XCurl original global wrapper transitions with engine boundaries."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from differential_xcurl_public_info import Profile,LOCK,TICK,ERROR
from unicorn import x86_const as X
class State(C.Structure):
    _fields_=[("engine",C.c_void_p),("references",C.c_uint32)]
INIT=C.CFUNCTYPE(C.c_uint32,C.POINTER(State),C.c_void_p)
DESTROY=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
FREE=C.CFUNCTYPE(None,C.c_void_p,C.c_uint32,C.c_void_p)
SHUTDOWN=C.CFUNCTYPE(None,C.c_void_p)
def main():
    dll,libpath=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img)
    lib=C.CDLL(str(libpath.resolve()))
    init=lib.xcurl_research_global_init
    init.argtypes=(C.POINTER(State),C.POINTER(Profile),C.c_void_p,LOCK,LOCK,TICK,INIT,ERROR,C.c_void_p)
    init.restype=C.c_uint32
    cleanup=lib.xcurl_research_global_cleanup
    cleanup.argtypes=(C.POINTER(State),C.POINTER(Profile),C.c_void_p,LOCK,LOCK,TICK,DESTROY,FREE,SHUTDOWN,C.c_void_p)
    cleanup.restype=None
    rng=random.Random(0x9e30)
    for action in ("init","cleanup"):
        for i in range(320):
            refs=rng.choice([0,0,1,1,2,3,10,0xffffffff])
            engine=rng.choice([0,0x76543000])
            first=rng.choice([0,rng.getrandbits(64)])
            now=rng.getrandbits(64)
            counters=[rng.getrandbits(32) for _ in range(24)]
            outcome=rng.choice([0,0,2,27])
            next_engine=0x76544000 if outcome==0 else engine
            oc=[];nc=[]
            def olock(mu,phase):
                assert mu.reg_read(X.UC_X86_REG_RCX)==img.base+0x28b58
                oc.append((phase,))
            def otick(mu):oc.append(("tick",));mu.reg_write(X.UC_X86_REG_RAX,now)
            def oinit(mu):
                oc.append(("init",))
                m.program_write(img.base+0x28c10,struct.pack("<Q",next_engine))
                return outcome
            def odestroy(mu):oc.append(("destroy",mu.reg_read(X.UC_X86_REG_RCX)));return 0
            def ofree(mu):oc.append(("free",mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
            def oshutdown(mu):oc.append(("shutdown",));return 0
            def oerror(mu):oc.append(("error",mu.reg_read(X.UC_X86_REG_ECX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
            for name in img.iat.values():
                if name.endswith("!EnterCriticalSection"):m.on_import(name,lambda mu:olock(mu,"enter"))
                if name.endswith("!LeaveCriticalSection"):m.on_import(name,lambda mu:olock(mu,"leave"))
                if name.endswith("!GetTickCount64"):m.on_import(name,otick)
            def setup(w):
                w.write(img.base+0x28c18,struct.pack("<I",refs))
                w.write(img.base+0x28c10,struct.pack("<Q",engine))
                w.write(img.base+0x28ba0,struct.pack("<Q",first))
                w.write(img.base+0x28bb0,struct.pack("<24I",*counters))
            ret,_,writes,_=m.call(img.exports["curl_global_"+action],[rng.getrandbits(32)] if action=="init" else [],
                "uint" if action=="init" else "void",setup=setup,
                fakes={img.base+r:cb for r,cb in [(0x9ba0,oinit),(0x165d0,odestroy),(0x9640,ofree),(0x5568,oshutdown),(0x83d8,oerror)]})
            assert not isinstance(ret,str),(action,i,ret)
            def updated(a,old):return b"".join(writes.get(img.base+a+j,bytes([v])) for j,v in enumerate(old))
            expected_ref=struct.unpack("<I",updated(0x28c18,struct.pack("<I",refs)))[0]
            expected_engine=struct.unpack("<Q",updated(0x28c10,struct.pack("<Q",engine)))[0]
            expected_first=struct.unpack("<Q",updated(0x28ba0,struct.pack("<Q",first)))[0]
            expected_counts=struct.unpack("<24I",updated(0x28bb0,struct.pack("<24I",*counters)))
            prof=Profile(first,(C.c_uint32*24)(*counters));state=State(engine,refs)
            sentinel=0x12345
            @LOCK
            def enter(p,ctx):assert p==sentinel;nc.append(("enter",))
            @LOCK
            def leave(p,ctx):assert p==sentinel;nc.append(("leave",))
            @TICK
            def tick(ctx):nc.append(("tick",));return now
            @INIT
            def ni(s,ctx):nc.append(("init",));s.contents.engine=next_engine;return outcome
            @DESTROY
            def nd(p,ctx):nc.append(("destroy",p or 0))
            @FREE
            def nf(p,tag,ctx):nc.append(("free",p or 0,tag))
            @SHUTDOWN
            def ns(ctx):nc.append(("shutdown",))
            @ERROR
            def ne(code,api,ctx):nc.append(("error",code,api))
            args=(C.byref(state),C.byref(prof),sentinel,enter,leave,tick)
            got=init(*args,ni,ne,None) if action=="init" else cleanup(*args,nd,nf,ns,None)
            assert ret==got and oc==nc,(action,i,refs,ret,got,oc,nc)
            assert (state.references,state.engine or 0,prof.first_tick,tuple(prof.counts))==(expected_ref,expected_engine,expected_first,expected_counts)
        print("XCurl original global "+action+" PASS: 320 refcount/profile/engine-boundary cases")
if __name__=="__main__":main()
