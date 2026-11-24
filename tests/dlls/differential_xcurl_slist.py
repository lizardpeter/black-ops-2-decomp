#!/usr/bin/env python3
"""Full original XCurl slist+duplicate+heap dispatch, with only process heap modeled."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from unicorn import x86_const as X
A=0x34000000
SIZE=0x100000
HEAP_HANDLE=0x76540000
ALLOC=C.CFUNCTYPE(C.c_void_p,C.c_size_t,C.c_void_p)
FREE=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
REALLOC=C.CFUNCTYPE(C.c_void_p,C.c_void_p,C.c_size_t,C.c_void_p)
def main():
    dll,libpath=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SIZE)
    lib=C.CDLL(str(libpath.resolve()))
    lib.xcurl_research_set_heap.argtypes=(ALLOC,FREE,REALLOC,C.c_void_p)
    append=lib.curl_slist_append;append.argtypes=(C.c_void_p,C.c_char_p);append.restype=C.c_void_p
    free=lib.curl_slist_free_all;free.argtypes=(C.c_void_p,);free.restype=None
    rng=random.Random(0x1bb80)
    arena=(C.c_uint8*SIZE)();B=C.addressof(arena)
    for case in range(192):
        oc=[];nc=[];of={};nf={};op=0x1000;np=0x1000
        failure=case%7
        def oalloc(mu):
            nonlocal op
            assert mu.reg_read(X.UC_X86_REG_RCX)==HEAP_HANDLE and mu.reg_read(X.UC_X86_REG_EDX)==0
            n=mu.reg_read(X.UC_X86_REG_R8)
            failed=len([x for x in oc if x[0]=="alloc"])==failure
            oc.append(("alloc",n,failed))
            if failed:mu.reg_write(X.UC_X86_REG_RAX,0);return
            p=A+op;op+=((n+63)&~63)+64
            of[p]=n;mu.mem_write(p,bytes([0xa5])*n)
            mu.reg_write(X.UC_X86_REG_RAX,p)
        def ofree(mu):
            assert mu.reg_read(X.UC_X86_REG_RCX)==HEAP_HANDLE and mu.reg_read(X.UC_X86_REG_EDX)==0
            p=mu.reg_read(X.UC_X86_REG_R8);n=of.pop(p)
            oc.append(("free",p-A,n,bytes(mu.mem_read(p,n)) if n!=16 else bytes(mu.mem_read(p,8))))
            mu.reg_write(X.UC_X86_REG_RAX,1)
        def heap(mu):mu.reg_write(X.UC_X86_REG_RAX,HEAP_HANDLE)
        for name in img.iat.values():
            if name.endswith("!GetProcessHeap"):m.on_import(name,heap)
            if name.endswith("!HeapAlloc"):m.on_import(name,oalloc)
            if name.endswith("!HeapFree"):m.on_import(name,ofree)
        @ALLOC
        def na(n,ctx):
            nonlocal np
            failed=len([x for x in nc if x[0]=="alloc"])==failure
            nc.append(("alloc",n,failed))
            if failed:return None
            p=B+np;np+=((n+63)&~63)+64
            nf[p]=n;C.memset(p,0xa5,n);return p
        @FREE
        def nr(p,ctx):
            n=nf.pop(p);nc.append(("free",p-B,n,C.string_at(p,n if n!=16 else 8)))
        @REALLOC
        def unused(p,n,ctx):raise AssertionError("unexpected realloc")
        lib.xcurl_research_set_heap(na,nr,unused,None)
        original=native=0
        words=[rng.randbytes(rng.choice([0,1,3,8,17,31,63,127,255,1023])).replace(b"\0",b"x") for _ in range(6)]
        # Distinct string lengths avoid confusing the 16-byte node and text.
        words=[w+b"x" if len(w)==15 else w for w in words]
        def call(name,args,kind):
            ret,_,_,_=m.call(img.exports[name],args,kind,limit=30_000_000)
            assert not isinstance(ret,str),(case,name,ret,m.imports_called)
            assert all(s.endswith(("!GetProcessHeap","!HeapAlloc","!HeapFree")) for s in m.imports_called),m.imports_called
            return ret
        def normalize(p,base):return p-base if p else 0
        def snapshot(p,base,read,active):
            result=[]
            while p:
                data,nxt=struct.unpack("<QQ",read(p,16))
                text=read(data,active[data])
                result.append((p-base,data-base,normalize(nxt,base),text))
                p=nxt
            return result
        for w in words:
            a=call("curl_slist_append",[original,w+b"\0"],"uint64")
            b=append(native,w)
            assert normalize(a,A)==normalize(b,B),(case,oc,nc,a,b)
            if a:original=a;native=b
            assert snapshot(original,A,lambda p,n:bytes(m.mu.mem_read(p,n)),of)==snapshot(native,B,C.string_at,nf)
            assert oc==nc,(case,oc,nc)
        call("curl_slist_free_all",[original],"void");free(native)
        assert oc==nc and not of and not nf,(case,oc,nc,of,nf)
    print("XCurl original slist/duplicate/default process heap PASS: 192 six-append/failure/cleanup cases")
if __name__=="__main__":main()
