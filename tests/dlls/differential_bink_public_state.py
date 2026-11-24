#!/usr/bin/env python3
"""Original Bink rectangle fast/cached branches and background-I/O dispatch.

Only the allocation boundary and supplied I/O callback are test doubles.
No dirty-map scan or real streaming I/O compatibility is asserted.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,SCRATCH,HEAP
from differential_bink_resident_state import PIN,SIZE,put,ptr
from unicorn import x86_const as X
ALLOC=ctypes.CFUNCTYPE(ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p)
IO=ctypes.CFUNCTYPE(ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32)
BUFFER=SCRATCH+0x1000
DIRTY=SCRATCH+0x2000

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img);lib=ctypes.CDLL(str(source.resolve()))
    rects=lib.bink_research_get_rects
    rects.argtypes=(ctypes.c_void_p,ctypes.c_uint32,ALLOC,ctypes.c_void_p)
    rects.restype=ctypes.c_int64
    io=lib.bink_research_control_background_io
    io.argtypes=(ctypes.c_void_p,ctypes.c_uint32);io.restype=ctypes.c_uint32
    rng=random.Random(0x1C820)
    for i in range(384):
        h=bytearray(rng.randbytes(SIZE))
        length=rng.randrange(256)
        flags=rng.choice([0,0x100000,0x4000000,0x1000,0x2000,0x80,0x80000])
        for off,val in ((0,rng.randrange(1,1281)),(4,rng.randrange(1,721)),
            (0x20,flags),(0x57C,rng.choice([0,0,0,1,2])),(0x114,length),
            (0x3C,rng.randrange(9))):
            put(h,off,val)
        allocated=i%2==0
        dirty=i%3==0
        ptr(h,0xD8,BUFFER if allocated else 0)
        ptr(h,0xE0,BUFFER if allocated else 0)
        ptr(h,0xF0,DIRTY if dirty else 0)
        initial=rng.randbytes(512)
        original_calls=[];native_calls=[]
        def alloc(mu):
            original_calls.append(mu.reg_read(X.UC_X86_REG_ECX))
            return BUFFER
        nb=(ctypes.c_uint8*len(initial)).from_buffer_copy(initial)
        @ALLOC
        def native_alloc(size,ctx):
            native_calls.append(size);return ctypes.addressof(nb)
        def setup(w):
            m.mu.mem_write(BUFFER,initial)
            m.mu.mem_write(DIRTY,bytes(512))
        fmt=rng.choice([0,0x80000,0x4000000,0x4080000,rng.getrandbits(32)])
        ret,bufs,_,_=m.call(img.exports["BinkGetRects"],[bytes(h),fmt],"uint",
            setup=setup,fakes={img.base+0x1010:alloc})
        assert isinstance(ret,int),(i,ret)
        native=bytearray(h)
        for off in (0xD8,0xE0):
            if struct.unpack_from("<Q",native,off)[0]==BUFFER:ptr(native,off,ctypes.addressof(nb))
        nh=(ctypes.c_uint8*SIZE).from_buffer(native)
        got=rects(nh,fmt,native_alloc,None)
        for off in (0xD8,0xE0):
            if struct.unpack_from("<Q",native,off)[0]==ctypes.addressof(nb):ptr(native,off,BUFFER)
        assert got==ret,(i,got,ret)
        assert bytes(native)==bufs[0],(i,"handle",[(hex(j),a,b) for j,(a,b) in enumerate(zip(native,bufs[0])) if a!=b][:20])
        assert original_calls==native_calls,(i,original_calls,native_calls)
        assert bytes(nb)==bytes(m.mu.mem_read(BUFFER,len(nb))),(i,"allocated bytes")
        assert not m.imports_called,m.imports_called
    assert rects(None,0,native_alloc,None)==0
    h=bytearray(SIZE);ptr(h,0xD8,BUFFER);ptr(h,0xF0,DIRTY);put(h,0x3C,0xffffffff)
    nh=(ctypes.c_uint8*SIZE).from_buffer(h)
    before=bytes(h)
    assert rects(nh,0,native_alloc,None)==-1 and bytes(h)==before
    print("Original BinkGetRects supported branches PASS: 384 complete handle/allocation cases")
    callback_rva=0x17950
    for i in range(192):
        h=bytearray(rng.randbytes(SIZE))
        enabled=i%3!=0
        ptr(h,0x220,img.base+callback_rva if enabled else 0)
        value=rng.getrandbits(32)
        returned=rng.getrandbits(32)
        original_calls=[];native_calls=[]
        def cb(mu):
            p=mu.reg_read(X.UC_X86_REG_RCX)
            v=mu.reg_read(X.UC_X86_REG_EDX)
            original_calls.append((p-HEAP,v))
            # Model callback mutation inside the IO state, not in dispatcher.
            mu.mem_write(p+8,struct.pack("<I",v))
            return returned
        ret,bufs,_,_=m.call(img.exports["BinkControlBackgroundIO"],[bytes(h),value],"uint",
                            fakes={img.base+callback_rva:cb})
        assert isinstance(ret,int),(i,ret)
        native=bytearray(h)
        nh=(ctypes.c_uint8*SIZE).from_buffer(native)
        @IO
        def native_cb(p,v):
            native_calls.append((p-ctypes.addressof(nh),v))
            ctypes.c_uint32.from_address(p+8).value=v
            return returned
        if enabled:ptr(native,0x220,ctypes.cast(native_cb,ctypes.c_void_p).value)
        got=io(nh,value)
        if enabled:ptr(native,0x220,img.base+callback_rva)
        assert got==ret and bytes(native)==bufs[0],(i,got,ret)
        assert original_calls==native_calls,(i,original_calls,native_calls)
        assert not m.imports_called,m.imports_called
    assert io(None,0)==0xffffffff
    print("Original BinkControlBackgroundIO dispatch PASS: 192 complete handle/callback cases")
if __name__=="__main__":main()
