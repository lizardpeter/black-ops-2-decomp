#!/usr/bin/env python3
"""Original async submit state, using real prepare and fake queue boundary.
No worker body or actual asynchronous execution is claimed."""
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
from unicorn import x86_const as X
ALLOC=ctypes.CFUNCTYPE(ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p)
QUEUE=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_void_p)
MAP1=SCRATCH+0x1000
MAP2=SCRATCH+0x2000
MAP3=SCRATCH+0x3000
def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img);lib=ctypes.CDLL(str(source.resolve()))
    fn=lib.bink_research_submit_async
    fn.argtypes=(ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,CLOCK,ctypes.c_void_p,
        ALLOC,ctypes.c_void_p,QUEUE,ctypes.c_void_p,ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_size_t),ctypes.c_size_t)
    fn.restype=ctypes.c_int
    idle=lib.bink_research_async_wait_idle
    idle.argtypes=(ctypes.c_void_p,);idle.restype=ctypes.c_int
    rng=random.Random(0x213f0)
    for case in range(512):
        h=bytearray(rng.randbytes(SIZE))
        length=rng.randrange(1,256)
        for off,val in ((0x144,0),(0x130,0),(0x0C,3),(0x56C,2 if case%7 else 3),
                       (0x270,0),(0x1C,0),(0x578,0),(0x128,rng.randrange(2)),
                       (0x134,1),(0x12C,0),(0x59C,0x694b4942),(0x114,length),
                       (0x120,0x301 if case%8==0 else 0),
                       (0x124,0x302 if case%9==0 else 0)):
            put(h,off,val)
        ptr(h,0xD0,0x12340000 if case%3 else 0)
        a=MAP1 if case%4 else 0
        b=[0,MAP1,MAP2][case%3]
        ptr(h,0xD8,a);ptr(h,0xE0,b)
        ptr(h,0xE8,0)
        now=rng.getrandbits(32)
        first=rng.randrange(8)
        second=first if case%3==0 else (first+1)%8
        accept=[rng.randrange(2),rng.randrange(2)]
        data=[rng.randbytes(512) for _ in range(3)]
        native=bytearray(h);nh=(ctypes.c_uint8*SIZE).from_buffer(native)
        buffers=[(ctypes.c_uint8*512).from_buffer_copy(v) for v in data]
        mapping=dict(zip((MAP1,MAP2,MAP3),[ctypes.addressof(v) for v in buffers]))
        reverse={v:k for k,v in mapping.items()}
        for off in (0xD8,0xE0,0xE8):
            p=struct.unpack_from("<Q",native,off)[0]
            if p in mapping:ptr(native,off,mapping[p])
        oc=[];nc=[];oa=[];na=[];oq=[];nq=[]
        def clock(mu):oc.append(now);return now
        @CLOCK
        def native_clock(ctx):nc.append(now);return now
        def alloc(mu):oa.append(mu.reg_read(X.UC_X86_REG_ECX));return MAP3
        @ALLOC
        def native_alloc(size,ctx):na.append(size);return mapping[MAP3]
        def queue(mu):
            worker=mu.reg_read(X.UC_X86_REG_ECX)
            message=struct.unpack("<Q",bytes(mu.mem_read(mu.reg_read(X.UC_X86_REG_RDX),8)))[0]
            assert mu.reg_read(X.UC_X86_REG_R8D)==8
            oq.append((worker,message-HEAP))
            return accept[len(oq)-1]
        @QUEUE
        def native_queue(worker,message,ctx):
            nq.append((worker,message-ctypes.addressof(nh)))
            return accept[len(nq)-1]
        err=rng.randbytes(256)
        entry=0x55550000
        def setup(w):
            for addr,value in zip((MAP1,MAP2,MAP3),data):m.mu.mem_write(addr,value)
            w.write(img.base+0x31860,err)
            w.write(img.base+0x57118,struct.pack("<Q",entry))
            w.write(img.base+0x57028,bytes(8))
        ret,bufs,writes,_=m.call(img.exports["BinkDoFrameAsync"],[bytes(h),first,second],"uint",
            setup=setup,fakes={img.base+0x1FF00:clock,img.base+0x1010:alloc,img.base+0x21F70:queue})
        assert isinstance(ret,int),(case,ret)
        e=(ctypes.c_uint8*256).from_buffer_copy(err);ep=ctypes.c_size_t(entry)
        got=fn(nh,first,second,native_clock,None,native_alloc,None,native_queue,None,e,ctypes.byref(ep),img.base+0x21810)
        for off in (0xD8,0xE0,0xE8):
            p=struct.unpack_from("<Q",native,off)[0]
            if p in reverse:ptr(native,off,reverse[p])
        assert got==ret,(case,got,ret)
        assert bytes(native)==bufs[0],(case,[(hex(i),a,b) for i,(a,b) in enumerate(zip(native,bufs[0])) if a!=b][:24])
        assert oc==nc and oa==na and oq==nq,(case,oc,nc,oa,na,oq,nq)
        for addr,buf in zip((MAP1,MAP2,MAP3),buffers):
            assert bytes(buf)==bytes(m.mu.mem_read(addr,len(buf))),(case,addr)
        expected=bytes(writes.get(img.base+0x31860+i,bytes([v]))[0] for i,v in enumerate(err))
        expected_entry=bytes(writes.get(img.base+0x57118+i,bytes([v]))[0] for i,v in enumerate(struct.pack("<Q",entry)))
        assert bytes(e)==expected,(case,"error")
        assert ep.value==struct.unpack("<Q",expected_entry)[0],(case,"worker entry")
        assert not m.imports_called
        # Wait's no-pending early return executes original, regardless of timeout.
        if not struct.unpack_from("<Q",native,0x120)[0]:
            r,bs,_,_=m.call(img.exports["BinkDoFrameAsyncWait"],[bytes(native),rng.getrandbits(32)],"uint")
            assert r==idle(nh)==1 and bs[0]==bytes(native)
    assert idle(None)==1
    r,_,_,_=m.call(img.exports["BinkDoFrameAsyncWait"],[0,0],"uint")
    assert r==1
    print("Original BinkDoFrameAsync dispatch PASS: 512 complete handle/queue cases; idle wait paths")
if __name__=="__main__":main()
