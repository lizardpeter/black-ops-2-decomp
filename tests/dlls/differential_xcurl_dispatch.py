#!/usr/bin/env python3
"""Original machine-code differential for 0xEAF8, disabled ETW.
Original heap and wide-conversion boundaries are observed, not simulated transport.
"""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from unicorn import x86_const as X
ALLOC=C.CFUNCTYPE(C.c_void_p,C.c_size_t,C.c_uint32,C.c_void_p)
FREE=C.CFUNCTYPE(None,C.c_void_p,C.c_uint32,C.c_void_p)
WIDE=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.c_void_p,C.c_uint32,C.c_void_p)
class Services(C.Structure):
    _fields_=[("allocate",ALLOC),("release",FREE),("wide_string",WIDE),("context",C.c_void_p)]
A=0x34000000;ARG=A+0x1000;STR=A+0x2000;DEST=A+0x3000
def main():
    dll,library=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000)
    lib=C.CDLL(str(library.resolve()))
    fn=lib.xcurl_research_dispatch_option
    fn.argtypes=(C.c_void_p,C.c_int,C.POINTER(C.c_uint64),C.POINTER(Services))
    fn.restype=C.c_uint32
    rng=random.Random(0xeaf8)
    # Include every known game selector, plus non-game recovered cases.
    options=[3,13,14,41,42,44,45,46,47,52,54,60,64,68,78,80,81,84,98,136,155,156,157,158,161,172,207,232,233,280,286,10001,10002,10009,10010,10015,10016,10018,10023,10024,10025,10029,10036,10037,10095,10097,10102,10103,10109,10165,10230,10240,10241,10269,10282,10284,10287,20011,20012,20079,20094,20108,20283,30115,30120,0,-1,9999]
    values=[0,1,2,4,5,30,0xffffffff,0xfffffffe,0x7fffffff,0x80000000,0x20c49b,0x20c49c,0x100000000,0xffffffffffffffff,0xfffffffffffffffe,0x8000]
    strings=[b"",b"abc",b"gzip",b"deflate",b"gzip,deflate",b"*",b"GZIP",b"https://example.invalid"]
    string_opts={10002,10016,10018,10025,10036,10102,10165}
    count=0
    failures=[]
    for opt in options:
      for j,val in enumerate(values):
        count+=1
        h=bytearray(rng.randbytes(0x110))
        # Owned pointer has a valid mapped address, or is null. Normalize
        # pointer fields to common integer tokens; never dereference old data.
        for off in (0x10,0x48,0x50,0x58,0x60,0x70):
            struct.pack_into("<Q",h,off,0x12345678 if j%2 else 0)
        h[0x100]=j%3
        struct.pack_into("<q",h,0x80,[-1,0,3,8,-2,0x100000000][j%6])
        payload=strings[j%len(strings)]+b"\0"*64
        source=C.create_string_buffer(payload);dest=C.create_string_buffer(256)
        native=C.create_string_buffer(bytes(h),len(h))
        base=C.addressof(native)
        source_ptr=C.addressof(source);dest_ptr=C.addressof(dest)
        if opt in string_opts:
            arg=STR if opt==10102 or j%4 else 0
            # Copy payload with -1 requires non-null string.
            if opt==10165:arg=STR
            native_arg=source_ptr if arg else 0
        else:arg=native_arg=val
        fail=j%5==0
        expected_wide=27 if fail else 0
        oc=[];nc=[]
        def oa(mu):
            size=mu.reg_read(X.UC_X86_REG_RCX);tag=mu.reg_read(X.UC_X86_REG_EDX)
            assert size<=256,(opt,size)
            oc.append(("alloc",size,tag));return 0 if fail else DEST
        def of(mu):
            oc.append(("free",mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
        def ow(mu):
            off=mu.reg_read(X.UC_X86_REG_RCX)-A
            ptr=mu.reg_read(X.UC_X86_REG_RDX)
            oc.append(("wide",off,bool(ptr),mu.reg_read(X.UC_X86_REG_R8D)))
            return expected_wide
        def setup(w):
            m.mu.mem_write(A,bytes(h));m.mu.mem_write(ARG,struct.pack("<Q",arg))
            m.mu.mem_write(STR,payload);m.mu.mem_write(DEST,bytes(256))
        ret,_,_,_=m.call(img.base+0xeaf8,[A,opt&0xffffffff,ARG],"uint",setup=setup,
            fakes={img.base+0x932c:oa,img.base+0x9640:of,img.base+0x1ac88:ow})
        assert isinstance(ret,int),(opt,j,ret)
        original=bytearray(m.mu.mem_read(A,len(h)))
        original_dest=bytes(m.mu.mem_read(DEST,256))
        @ALLOC
        def na(size,tag,ctx):nc.append(("alloc",size,tag));return None if fail else dest_ptr
        @FREE
        def nf(ptr,tag,ctx):nc.append(("free",ptr,tag))
        @WIDE
        def nw(slot,ptr,tag,ctx):
            nc.append(("wide",slot-base,bool(ptr),tag));return expected_wide
        services=Services(na,nf,nw,None)
        got=fn(native,opt,C.byref(C.c_uint64(native_arg)),C.byref(services))
        result=bytearray(bytes(native))
        for off in (0x10,0x48):
            if struct.unpack_from("<Q",result,off)[0]==dest_ptr:struct.pack_into("<Q",result,off,DEST)
        if got!=ret or nc!=oc or result!=original or bytes(dest)!=original_dest:
            failure=(opt,j,got,ret,nc,oc,[(i,a,b) for i,(a,b) in enumerate(zip(result,original)) if a!=b],bytes(dest)!=original_dest)
            failures.append(failure)
            print("DISPATCH MISMATCH",failure,flush=True)
    assert not failures, f"{len(failures)} of {count} mismatches"
    print(f"XCurl original dispatcher PASS: {count} cases, {len(options)} selectors")
if __name__=="__main__":main()
