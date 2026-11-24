#!/usr/bin/env python3
"""Compare full resident Bink handle memory against pinned ORIGINAL x64 code.

Only external clock reads are replaced by deterministic values. Original
BinkNextFrame runs its real internal 0x16440 and no-pending-I/O 0x162D0
paths; neither is hooked. Audio-free, resident storage, telemetry disabled.
Native reconstruction uses the same 0x5A8 field layout with its own table.
"""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,SCRATCH
from unicorn import x86_const as X

PIN="10e67763420dd5c2654648d1b0242ba8aceac14ac9211c202360a3f12219af17"
INDEX=SCRATCH+0x1000
RESIDENT=0x32000000
SIZE=0x5A8
CLOCK=ctypes.CFUNCTYPE(ctypes.c_uint32,ctypes.c_void_p)
def put(h,off,v):struct.pack_into("<I",h,off,v)
def ptr(h,off,v):struct.pack_into("<Q",h,off,v)

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll)
    m=Machine(img)
    lib=ctypes.CDLL(str(source.resolve()))
    select=lib.bink_research_select_resident_frame
    select.argtypes=(ctypes.c_void_p,ctypes.c_uint32,CLOCK,ctypes.c_void_p)
    select.restype=ctypes.c_int
    advance=lib.bink_research_next_resident_frame
    advance.argtypes=(ctypes.c_void_p,CLOCK,ctypes.c_void_p)
    advance.restype=ctypes.c_int
    rng=random.Random(0x16440)
    checked=0
    for count in (1,2,5,31,173):
        offsets=[100]
        for i in range(count):offsets.append(offsets[-1]+rng.randrange(2,300)*4)
        offsets=[v|int(i==0 or (i<count and rng.randrange(4)==0)) for i,v in enumerate(offsets)]
        table=(ctypes.c_uint32*len(offsets))(*offsets)
        packed=struct.pack(f"<{len(offsets)}I",*offsets)
        def setup(w):
            # Scratch rather than image: index is read-only and never a test double.
            m.mu.mem_write(INDEX,packed)
        for current in sorted({1,count,max(1,count//2)}):
            for timestamp in (0,1,0xffffffff,1234,0xfffffff0):
                for busy in (0,1,2):
                    h=bytearray(rng.randbytes(SIZE))
                    for off,val in ((8,count),(0x0C,current),(0x120,int(busy==1)),
                        (0x124,int(busy==2)),(0x138,0),(0x144,0),
                        (0x530,timestamp),(0x574,timestamp),(0x57C,0x1234)):
                        put(h,off,val)
                    ptr(h,0x1A0,INDEX)
                    ptr(h,0x1D8,RESIDENT)
                    now=rng.choice([0,0xffffffff,300001,500000,0x87654321])
                    for target in (None,0,1,count):
                        if busy and target is not None:continue
                        calls=[]
                        native_calls=[]
                        def orig_clock(mu):
                            calls.append(now)
                            return now
                        @CLOCK
                        def clock(ctx):
                            native_calls.append(now)
                            return now
                        args=[bytes(h)] if target is None else [bytes(h),target]
                        va=img.exports["BinkNextFrame"] if target is None else img.base+0x16440
                        ret,bufs,_,_=m.call(va,args,"void",setup=setup,
                                            fakes={img.base+0x1FF00:orig_clock})
                        assert ret is None,(count,current,timestamp,busy,target,ret)
                        native=bytearray(h)
                        ptr(native,0x1A0,ctypes.addressof(table))
                        n=(ctypes.c_uint8*SIZE).from_buffer(native)
                        status=advance(n,clock,None) if target is None else select(n,target,clock,None)
                        assert status==(1 if busy else 0)
                        ptr(native,0x1A0,INDEX)
                        if bytes(native)!=bufs[0]:
                            diff=[i for i,(a,b) in enumerate(zip(native,bufs[0])) if a!=b]
                            raise AssertionError((count,current,timestamp,busy,target,now,diff[:32],
                                [(hex(i),native[i],bufs[0][i]) for i in diff[:16]]))
                        assert calls==native_calls,(calls,native_calls)
                        assert not m.imports_called,m.imports_called
                        checked+=1
    assert advance(None,clock,None)==0
    ret,_,_,_=m.call(img.exports["BinkNextFrame"],[0],"void")
    assert ret is None
    print(f"Resident frame-state differential PASS: {checked} complete handle comparisons + null")
if __name__=="__main__":main()
