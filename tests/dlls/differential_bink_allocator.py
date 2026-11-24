#!/usr/bin/env python3
"""Original custom allocation and free callbacks, exact overhead/footer."""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,SCRATCH
from differential_bink_resident_state import PIN
from unicorn import x86_const as X
ALLOC=ctypes.CFUNCTYPE(ctypes.c_void_p,ctypes.c_size_t)
FREE=ctypes.CFUNCTYPE(None,ctypes.c_void_p)

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img);lib=ctypes.CDLL(str(source.resolve()))
    allocate=lib.bink_research_allocate_custom
    allocate.argtypes=(ctypes.c_uint32,ALLOC,FREE);allocate.restype=ctypes.c_void_p
    free=lib.bink_research_free_custom
    free.argtypes=(ctypes.c_void_p,);free.restype=ctypes.c_int
    alloc_rva=0x17950
    free_rva=0x17970
    rng=random.Random(0x1010)
    storage=(ctypes.c_uint8*8192)()
    native_base=(ctypes.addressof(storage)+31)&~31
    original_base=SCRATCH+0x1000
    for i in range(320):
        offset=i%32
        size=rng.randrange(1,2048)
        mode=i%10
        original_raw=original_base+offset
        native_raw=native_base+offset
        initial=rng.randbytes(4096)
        original_calls=[];native_calls=[]
        def oa(mu):
            original_calls.append(("alloc",mu.reg_read(X.UC_X86_REG_ECX)))
            return 0 if mode==0 else (0xffffffffffffffff if mode==1 else original_raw)
        def of(mu):original_calls.append(("free",mu.reg_read(X.UC_X86_REG_RCX)-original_base));return 0
        @ALLOC
        def na(n):
            native_calls.append(("alloc",n))
            return 0 if mode==0 else (0xffffffffffffffff if mode==1 else native_raw)
        @FREE
        def nf(p):native_calls.append(("free",p-native_base))
        def setup(w):
            w.write(img.base+0x542A8,struct.pack("<Q",img.base+alloc_rva))
            w.write(img.base+0x542B0,struct.pack("<Q",img.base+free_rva))
            m.mu.mem_write(original_base,initial)
        r,_,_,_=m.call(img.base+0x1010,[size],"uint64",setup=setup,
                       fakes={img.base+alloc_rva:oa})
        ctypes.memmove(native_base,initial,len(initial))
        result=allocate(size,na,nf)
        assert isinstance(r,int) and original_calls==native_calls,(i,r,original_calls,native_calls)
        if mode in (0,1):
            assert r==0 and result is None
            continue
        assert result-native_base==r-original_base
        original=bytes(m.mu.mem_read(original_base,len(initial)))
        native=bytearray(ctypes.string_at(native_base,len(initial)))
        at=result-native_base
        assert native[at-2]==3 and native[at-1]==64-offset
        assert struct.unpack_from("<Q",native,at-12)[0]==ctypes.cast(nf,ctypes.c_void_p).value
        struct.pack_into("<Q",native,at-12,img.base+free_rva)
        assert bytes(native)==original,(i,"footer")
        def setup_free(w):
            m.mu.mem_write(original_base,original)
            # Current global free callback must not replace allocation's
            # captured footer callback.
            w.write(img.base+0x542B0,bytes(8))
        rr,_,_,_=m.call(img.base+0x10d0,[r],"void",setup=setup_free,fakes={img.base+free_rva:of})
        assert rr is None and free(result)==0 and original_calls==native_calls
        assert ctypes.string_at(native_base,len(initial))==bytes(native[:at-12])+struct.pack("<Q",ctypes.cast(nf,ctypes.c_void_p).value)+bytes(native[at-4:])
    @ALLOC
    def unexpected(n):raise AssertionError("zero/invalid request allocated")
    @FREE
    def unused(p):raise AssertionError("unexpected free")
    assert allocate(0,unexpected,unused) is None and allocate(0xffffffff,unexpected,unused) is None
    assert free(None)==0
    print("Original custom Bink allocator PASS: 320 full footer and callback cases")
if __name__=="__main__":main()
