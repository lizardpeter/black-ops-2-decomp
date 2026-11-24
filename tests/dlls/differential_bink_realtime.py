#!/usr/bin/env python3
"""BinkGetRealtime 0x1C290: original full handle and 0x38 output differential."""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,SCRATCH
from differential_bink_resident_state import PIN,SIZE,CLOCK,put,ptr

RING_OFFSETS=(0x4D8,0x4E8,0x4E0,0x4F8,0x500,0x508,0x4F0)
RINGS=[SCRATCH+0x3000+i*0x1000 for i in range(7)]
TABLE=SCRATCH+0x1000

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll); m=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    fn=lib.bink_research_get_realtime
    fn.argtypes=(ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32,CLOCK,ctypes.c_void_p)
    fn.restype=ctypes.c_int
    rng=random.Random(0x1C290)
    for case in range(896):
        h=bytearray(rng.randbytes(SIZE))
        n=rng.choice([2,3,4,8,32,64]) if case<640 else rng.choice([1,2,3,32])
        total=rng.randrange(2,80)
        frame=rng.randrange(2,total+1) if case<640 else rng.choice([1,2,total])
        request=rng.choice([0,1,2,n-1,n,n+1,frame,0xffffffff])
        for off,val in ((0x08,total),(0x0C,frame),(0x10,rng.randrange(frame+1)),
                       (0x14,rng.getrandbits(32)),(0x18,rng.getrandbits(32)),
                       (0x510,n),(0x514,rng.randrange(n)),(0x560,rng.choice([30,10000000,0xffffffff])),
                       (0x564,rng.choice([1,1001,333333,0xffffffff])),
                       (0x530,rng.choice([0,1,0xffffffff,0xfffffff0,123]))):
            put(h,off,val)
        # Preserve random key bits and large wrapped 32-bit products.
        offset=rng.randrange(100,100000)&~1
        entries=[]
        for i in range(total+1):
            entries.append(offset|rng.randrange(2))
            offset=(offset+2*rng.randrange(1,100000))&0xffffffff
        data=struct.pack(f"<{len(entries)}I",*entries)
        ptr(h,0x1A0,TABLE)
        rings=[rng.randbytes(4*n+16) for _ in range(7)]
        if case>=640 and case%2==0:rings[0]=bytes(4*n+16)
        for off,addr in zip(RING_OFFSETS,RINGS):ptr(h,off,addr)
        out=rng.randbytes(0x38+16)
        now=rng.getrandbits(32)
        original_calls=[];native_calls=[]
        def timer(mu):
            original_calls.append(now);return now
        @CLOCK
        def clock(ctx):
            native_calls.append(now);return now
        def setup(w):
            m.mu.mem_write(TABLE,data)
            for addr,ring in zip(RINGS,rings):m.mu.mem_write(addr,ring)
        ret,bufs,_,_=m.call(img.exports["BinkGetRealtime"],[bytes(h),out,request],
                            "void",setup=setup,fakes={img.base+0x1FF00:timer})
        assert ret is None,(case,ret)
        native=bytearray(h)
        t=(ctypes.c_uint8*len(data)).from_buffer_copy(data)
        rs=[(ctypes.c_uint8*len(r)).from_buffer_copy(r) for r in rings]
        ptr(native,0x1A0,ctypes.addressof(t))
        for off,r in zip(RING_OFFSETS,rs):ptr(native,off,ctypes.addressof(r))
        nh=(ctypes.c_uint8*SIZE).from_buffer(native)
        no=(ctypes.c_uint8*len(out)).from_buffer_copy(out)
        assert fn(nh,no,request,clock,None)==0
        ptr(native,0x1A0,TABLE)
        for off,addr in zip(RING_OFFSETS,RINGS):ptr(native,off,addr)
        assert bytes(native)==bufs[0],(case,"handle")
        assert bytes(no)==bufs[1],(case,request,frame,n,
            struct.unpack("<18I",bytes(no)),struct.unpack("<18I",bufs[1]))
        assert original_calls==native_calls==[now]
        assert bytes(t)==data and all(bytes(r)==v for r,v in zip(rs,rings))
        assert not m.imports_called,m.imports_called
    # Both null cases must return without invoking the timer or writing.
    @CLOCK
    def unused(ctx):raise AssertionError("unexpected timer")
    o=(ctypes.c_uint8*72)(*([0xA5]*72))
    h=(ctypes.c_uint8*SIZE)()
    assert fn(None,o,1,unused,None)==0 and bytes(o)==bytes([0xA5])*72
    assert fn(h,None,1,unused,None)==0
    print("BinkGetRealtime differential PASS: 896 complete handle/output cases + null paths")
if __name__=="__main__":main()
