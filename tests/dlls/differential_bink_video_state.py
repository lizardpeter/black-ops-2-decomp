#!/usr/bin/env python3
"""Original Bink pause/prepare/finish handle, dirty-map and ring comparison.

Real original 0x1BEF0 / 0x1A260 / 0x1A380 execute under Unicorn.
Only the platform timer is replaced; dirty-map memset and all original
state writes execute. No audio or original asynchronous worker is modeled.
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
from differential_bink_resident_state import PIN,SIZE,CLOCK,put,ptr
RING_OFFSETS=(0x4D8,0x4E8,0x4E0,0x4F0,0x4F8,0x500,0x508)
DESC=SCRATCH+0x1000
DIRTY=SCRATCH+0x2000
RINGS=[SCRATCH+0x3000+i*0x1000 for i in range(7)]
DIRTY_N=128

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll); m=Machine(img); lib=ctypes.CDLL(str(source.resolve()))
    pause=lib.bink_research_pause_video_only
    pause.argtypes=(ctypes.c_void_p,ctypes.c_uint32,CLOCK,ctypes.c_void_p)
    pause.restype=ctypes.c_int64
    prep=lib.bink_research_prepare_video_frame
    prep.argtypes=(ctypes.c_void_p,ctypes.c_uint32);prep.restype=ctypes.c_int
    finish=lib.bink_research_finish_video_frame
    finish.argtypes=(ctypes.c_void_p,ctypes.c_uint32,CLOCK,ctypes.c_void_p);finish.restype=ctypes.c_int
    rng=random.Random(0x1A380)
    for kind,iterations in (("pause",192),("prepare",192),("finish",384)):
        for case in range(iterations):
            h=bytearray(rng.randbytes(SIZE))
            for off,val in ((0x144,0),(0x130,0),(0x0C,rng.randrange(1,15)),
                           (0x128,rng.choice([0,1,2,0xffffffff])),
                           (0x530,rng.choice([0,1,0xffffffff,0xfffffff0,123])),
                           (0x534,rng.choice([0,1,123,0xffffffff])),
                           (0x570,rng.choice([0,1,123,0xffffffff])),
                           (0x578,rng.randrange(2)),(0x270,rng.randrange(2)),
                           (0x1C,rng.randrange(2)),(0x12C,rng.randrange(2)),
                           (0x134,rng.randrange(2)),(0x114,DIRTY_N),
                           (0x59C,rng.choice([0x6132424B,0x694b4942])),
                           (0x524,rng.choice([0,1234])),
                           (0x560,rng.choice([1,30,10000000])),
                           (0x564,rng.choice([1,333333,1001]))):
                put(h,off,val)
            frame=struct.unpack_from("<I",h,0x0C)[0]
            put(h,0x56C,frame if case%7==0 else frame-1)
            # Ensure most prep cases actually enter the body and dirty-map path.
            if kind=="prepare" and case%3!=0:
                put(h,0x270,0);put(h,0x1C,0);put(h,0x134,1);put(h,0x12C,0)
                put(h,0x59C,0x694b4942)
            slot=rng.randrange(2)
            descriptor=bytearray(rng.randbytes(0xD8))
            put(descriptor,0x14,slot)
            for i in (0,1):ptr(descriptor,0x20+i*0x60,
                rng.choice([0,0x65432100]) if i!=slot else 0x65432100)
            has_desc=case%5!=0
            has_dirty=case%4!=0
            ptr(h,0xD0,DESC if has_desc else 0)
            ptr(h,0xE0,DIRTY if has_dirty else 0)
            ptr(h,0xE8,DIRTY if has_dirty else 0)
            ptr(h,0xF0,DIRTY if case%2 else 0)
            dirty=rng.randbytes(DIRTY_N+32)
            n=rng.choice([1,2,4,30])
            put(h,0x510,n);put(h,0x514,rng.randrange(n))
            ring_data=[rng.randbytes((n+2)*4) for _ in range(7)]
            for off,addr in zip(RING_OFFSETS,RINGS):ptr(h,off,addr)
            def setup(w):
                m.mu.mem_write(DESC,bytes(descriptor))
                m.mu.mem_write(DIRTY,dirty)
                for addr,data in zip(RINGS,ring_data):m.mu.mem_write(addr,data)
            clocks=[rng.getrandbits(32),rng.getrandbits(32)]
            observed=[];native_clocks=[]
            def oc(mu):
                v=clocks[len(observed)];observed.append(v);return v
            @CLOCK
            def nc(ctx):
                v=clocks[len(native_clocks)];native_clocks.append(v);return v
            now=rng.getrandbits(32)
            value=rng.choice([0,1,2,0xffffffff]) if kind=="pause" else now
            va={"pause":img.exports["BinkPause"],"prepare":img.base+0x1A260,"finish":img.base+0x1A380}[kind]
            ret,bufs,_,_=m.call(va,[bytes(h),value],
                "uint64" if kind=="prepare" else "uint",setup=setup,
                fakes={img.base+0x1FF00:oc})
            assert isinstance(ret,int),(kind,case,ret)
            assert not m.imports_called,m.imports_called
            native=bytearray(h)
            db=(ctypes.c_uint8*len(descriptor)).from_buffer_copy(descriptor)
            dirtybuf=(ctypes.c_uint8*len(dirty)).from_buffer_copy(dirty)
            rings=[(ctypes.c_uint8*len(data)).from_buffer_copy(data) for data in ring_data]
            maps={DESC:ctypes.addressof(db),DIRTY:ctypes.addressof(dirtybuf)}
            maps.update({addr:ctypes.addressof(buf) for addr,buf in zip(RINGS,rings)})
            ptr_offsets=(0xD0,0xE0,0xE8,0xF0)+RING_OFFSETS
            for off in ptr_offsets:
                p=struct.unpack_from("<Q",native,off)[0]
                if p in maps:ptr(native,off,maps[p])
            nh=(ctypes.c_uint8*SIZE).from_buffer(native)
            got=pause(nh,value,nc,None) if kind=="pause" else (
                prep(nh,value) if kind=="prepare" else finish(nh,value,nc,None))
            assert got==(int(ret!=0) if kind=="prepare" else ret),(kind,case,got,ret)
            reverse={v:k for k,v in maps.items()}
            for off in ptr_offsets:
                p=struct.unpack_from("<Q",native,off)[0]
                if p in reverse:ptr(native,off,reverse[p])
            if bytes(native)!=bufs[0]:
                diff=[i for i,(a,b) in enumerate(zip(native,bufs[0])) if a!=b]
                raise AssertionError((kind,case,[(hex(i),native[i],bufs[0][i]) for i in diff[:32]]))
            assert observed==native_clocks,(kind,case,observed,native_clocks)
            for addr,buf in [(DESC,db),(DIRTY,dirtybuf),*zip(RINGS,rings)]:
                assert bytes(buf)==bytes(m.mu.mem_read(addr,len(buf))),(kind,case,hex(addr))
        print(f"Original {kind} state differential PASS: {iterations} complete handle/buffer cases")
if __name__=="__main__":main()
