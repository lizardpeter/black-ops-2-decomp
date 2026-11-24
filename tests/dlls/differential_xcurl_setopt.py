#!/usr/bin/env python3
"""Differential of original public/locked setopt control flow, with explicit dispatcher boundary."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from differential_xcurl_public_info import Profile,LOCK,TICK,ERROR
from unicorn import x86_const as X
DISPATCH=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.c_int,C.POINTER(C.c_uint64),C.c_void_p)
ACTIVE=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
A=0x34000000
def main():
    dll,libpath=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000)
    lib=C.CDLL(str(libpath.resolve()))
    fn=lib.xcurl_research_easy_setopt
    fn.argtypes=(C.c_void_p,C.c_int,C.c_uint64,C.POINTER(Profile),C.c_void_p,LOCK,LOCK,TICK,DISPATCH,ACTIVE,ERROR,C.c_void_p)
    fn.restype=C.c_uint32
    rng=random.Random(0xe460)
    options=[60,14,20012,10009,10002,44,46,10036,10023,155,41,42,10010,20011,10001,20079,10029,20094,-1,0x7fffffff]
    for i in range(640):
        option=options[i%len(options)];arg=rng.getrandbits(64)
        present=(i//len(options))%4!=0
        active=(i//len(options))%4==1
        outcome=[0,4,0x30,2,27,43,99,0xffffffff][(i//len(options))%8]
        h=bytearray(rng.randbytes(0x320));struct.pack_into("<Q",h,0x310,0x12345678 if active else 0)
        first=0 if i%3==0 else rng.getrandbits(64)
        now=rng.getrandbits(64)
        counts=[rng.getrandbits(32) for _ in range(24)]
        if i%5==0:counts[7]=0xffffffff
        oc=[];nc=[]
        def olock(mu,phase):
            address=mu.reg_read(X.UC_X86_REG_RCX)
            assert address in (img.base+0x28b58,A+0x10,A+0x50)
            oc.append((phase,"profile" if address==img.base+0x28b58 else address-A))
        def otick(mu):oc.append(("tick",));mu.reg_write(X.UC_X86_REG_RAX,now)
        def odispatch(mu):
            assert mu.reg_read(X.UC_X86_REG_RCX)==A+0x78
            o=C.c_int32(mu.reg_read(X.UC_X86_REG_EDX)).value
            a=struct.unpack("<Q",mu.mem_read(mu.reg_read(X.UC_X86_REG_R8),8))[0]
            oc.append(("dispatch",o,a));return outcome
        def oactive(mu):
            assert mu.reg_read(X.UC_X86_REG_RCX)==A
            oc.append(("active",));return 0
        def oerror(mu):oc.append(("error",mu.reg_read(X.UC_X86_REG_ECX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
        for name in img.iat.values():
            if name.endswith("!EnterCriticalSection"):m.on_import(name,lambda mu:olock(mu,"enter"))
            if name.endswith("!LeaveCriticalSection"):m.on_import(name,lambda mu:olock(mu,"leave"))
            if name.endswith("!GetTickCount64"):m.on_import(name,otick)
        def setup(w):
            w.write(img.base+0x28ba0,struct.pack("<Q",first))
            w.write(img.base+0x28bb0,struct.pack("<24I",*counts))
            m.mu.mem_write(A,bytes(h))
        ret,_,writes,_=m.call(img.exports["curl_easy_setopt"],[A if present else 0,option&0xffffffff,arg],"uint",
            setup=setup,fakes={img.base+0xeaf8:odispatch,img.base+0xa224:oactive,img.base+0x83d8:oerror})
        assert isinstance(ret,int),(i,ret,m.imports_called)
        def updated(a,old):return b"".join(writes.get(img.base+a+j,bytes([v])) for j,v in enumerate(old))
        expected_first=struct.unpack("<Q",updated(0x28ba0,struct.pack("<Q",first)))[0]
        expected_counts=struct.unpack("<24I",updated(0x28bb0,struct.pack("<24I",*counts)))
        native=C.create_string_buffer(bytes(h),len(h));base=C.addressof(native)
        profile=Profile(first,(C.c_uint32*24)(*counts));sentinel=0x12345
        def nlock(p,phase):
            assert p in (sentinel,base+0x10,base+0x50)
            nc.append((phase,"profile" if p==sentinel else p-base))
        @LOCK
        def en(p,c):nlock(p,"enter")
        @LOCK
        def le(p,c):nlock(p,"leave")
        @TICK
        def tick(c):nc.append(("tick",));return now
        @DISPATCH
        def dispatch(p,o,a,c):
            assert p==base+0x78
            nc.append(("dispatch",o,a[0]));return outcome
        @ACTIVE
        def busy(p,c):
            assert p==base
            nc.append(("active",))
        @ERROR
        def error(code,api,c):nc.append(("error",code,api))
        got=fn(native if present else None,option,arg,C.byref(profile),sentinel,en,le,tick,dispatch,busy,error,None)
        assert got==ret and oc==nc,(i,option,got,ret,oc,nc)
        assert (profile.first_tick,tuple(profile.counts))==(expected_first,expected_counts)
        assert bytes(native)==bytes(h) and bytes(m.mu.mem_read(A,len(h)))==bytes(h)
    print("XCurl original public/locked setopt PASS: 640 option/profile/locking/active-request/error boundary cases")
if __name__=="__main__":main()
