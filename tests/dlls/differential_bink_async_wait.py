#!/usr/bin/env python3
"""Original async wait, cross-handle receipt, buffer swap and real finish."""
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
RING_OFFSETS=(0x4D8,0x4E8,0x4E0,0x4F0,0x4F8,0x500,0x508)
RINGS=[SCRATCH+0x3000+i*0x1000 for i in range(7)]
DESC=SCRATCH+0x1000
DIRTY1=SCRATCH+0x2000
DIRTY2=SCRATCH+0x2100
OTHER=SCRATCH+0xB000
class Completion(ctypes.Structure):
    _fields_=[("tagged_handle",ctypes.c_size_t),("result",ctypes.c_int64)]
RECEIVE=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_int32,
    ctypes.POINTER(Completion),ctypes.POINTER(ctypes.c_char_p),ctypes.c_void_p)
NOTIFY=ctypes.CFUNCTYPE(None,ctypes.c_uint32,ctypes.c_void_p)

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img);lib=ctypes.CDLL(str(source.resolve()))
    fn=lib.bink_research_wait_async
    fn.argtypes=(ctypes.c_void_p,ctypes.c_int32,ctypes.c_uint32,ctypes.c_uint32,
                 CLOCK,ctypes.c_void_p,RECEIVE,NOTIFY,ctypes.c_void_p,ctypes.c_void_p)
    fn.restype=ctypes.c_int
    rng=random.Random(0x21590)
    for case in range(512):
        h=bytearray(rng.randbytes(SIZE))
        other=bytearray(rng.randbytes(SIZE))
        first=case%8;second=(first+1)%8
        mode=case%8
        timeout=[0,1,20,0,-1,0,1,0][mode]
        # Case 0: idle; 1: no completion/positive timeout; 2: both slots
        # complete; 3: cross-handle, own completion; 4: blocking success;
        # 5: invalid worker; 6: out-of-range worker; 7: already acknowledged.
        a,b=(0,0) if mode==0 else (first|0x300,0)
        if mode==2:b=second|0x300
        if mode==6:a=8|0x300
        if mode==7:a=first|0x200
        for off,val in ((0x120,a),(0x124,b),(0x144,0),(0x510,4),(0x514,rng.randrange(4)),
                        (0x560,30),(0x564,1),(0x524,0 if case%3 else 1234),
                        (0x534,rng.choice([0,1,123])),(0x570,rng.getrandbits(32)),
                        (0x0C,3),(0x56C,2),(0x520,2)):
            put(h,off,val)
        put(other,0x120,first|0x300);put(other,0x124,second|0x300)
        desc=bytearray(rng.randbytes(0xD8));put(desc,0x14,case%2)
        for slot in range(2):ptr(desc,0x20+slot*0x60,0x12340000)
        ptr(h,0xD0,DESC if case%3 else 0)
        ptr(h,0xD8,DIRTY1 if case%2 else 0)
        ptr(h,0xE0,DIRTY2 if case%3 else 0)
        ptr(h,0xE8,DIRTY2 if case%3 else 0)
        ptr(h,0xF0,0)
        ringdata=[rng.randbytes(32) for _ in RINGS]
        for off,addr in zip(RING_OFFSETS,RINGS):ptr(h,off,addr)
        active=0 if mode in (5,6) else 0xff
        running=active
        plans={}
        if mode==2:plans={first:[(HEAP,13)],second:[(HEAP|1,42)]}
        if mode==3:plans={first:[(OTHER|1,-1),(HEAP,7)]}
        if mode==4:plans={first:[(HEAP,0)]}
        errors=bytearray(rng.randbytes(256))
        clock0=rng.getrandbits(32)
        oc=[];nc=[];oq=[];nq=[];on=[];nn=[]
        def clock(mu):
            t=(clock0+30*len(oc))&0xffffffff
            oc.append(t);return t
        @CLOCK
        def native_clock(ctx):
            t=(clock0+30*len(nc))&0xffffffff
            nc.append(t);return t
        op={k:list(v) for k,v in plans.items()}
        np={k:list(v) for k,v in plans.items()}
        def receive(mu):
            base=mu.reg_read(X.UC_X86_REG_RCX)
            worker=(base-(img.base+0x3C860))//0x14D0
            delay=ctypes.c_int32(mu.reg_read(X.UC_X86_REG_EDX)).value
            assert mu.reg_read(X.UC_X86_REG_R9D)==16
            oq.append((worker,delay))
            queue=op.get(worker,[])
            if not queue:return 0
            tag,value=queue.pop(0)
            mu.mem_write(mu.reg_read(X.UC_X86_REG_R8),struct.pack("<Qq",tag,value))
            return 1
        def notify(mu):on.append(mu.reg_read(X.UC_X86_REG_ECX));return 1
        native=bytearray(h)
        nh=(ctypes.c_uint8*SIZE).from_buffer(native)
        no=(ctypes.c_uint8*SIZE).from_buffer_copy(other)
        db=(ctypes.c_uint8*len(desc)).from_buffer_copy(desc)
        rings=[(ctypes.c_uint8*len(v)).from_buffer_copy(v) for v in ringdata]
        dirty1=(ctypes.c_uint8*256)();dirty2=(ctypes.c_uint8*256)()
        mapping={DESC:ctypes.addressof(db),DIRTY1:ctypes.addressof(dirty1),
                 DIRTY2:ctypes.addressof(dirty2),HEAP:ctypes.addressof(nh),OTHER:ctypes.addressof(no)}
        mapping.update({addr:ctypes.addressof(b) for addr,b in zip(RINGS,rings)})
        reverse={v:k for k,v in mapping.items()}
        ptr_offsets=(0xD0,0xD8,0xE0,0xE8,0xF0)+RING_OFFSETS
        for off in ptr_offsets:
            addr=struct.unpack_from("<Q",native,off)[0]
            if addr in mapping:ptr(native,off,mapping[addr])
        @RECEIVE
        def native_receive(worker,delay,result,err,ctx):
            nq.append((worker,delay))
            queue=np.get(worker,[])
            if not queue:return 0
            tag,value=queue.pop(0)
            result.contents.tagged_handle=mapping[tag&~1]|(tag&1)
            result.contents.result=value
            return 1
        @NOTIFY
        def native_notify(worker,ctx):nn.append(worker)
        def setup(w):
            m.mu.mem_write(DESC,bytes(desc))
            m.mu.mem_write(OTHER,bytes(other))
            for addr,data in zip(RINGS,ringdata):m.mu.mem_write(addr,data)
            w.write(img.base+0x31860,bytes(errors))
            w.write(img.base+0x570BC,struct.pack("<I",active))
            w.write(img.base+0x570A4,struct.pack("<I",running))
            w.write(img.base+0x57028,bytes(8))
        ret,bufs,writes,_=m.call(img.exports["BinkDoFrameAsyncWait"],[bytes(h),timeout],"uint",
            setup=setup,fakes={img.base+0x1FF00:clock,img.base+0x21BE0:receive,img.base+0x21FF0:notify})
        assert isinstance(ret,int),(case,ret)
        err=(ctypes.c_uint8*256).from_buffer_copy(errors)
        got=fn(nh,timeout,active,running,native_clock,None,native_receive,native_notify,None,err)
        for off in ptr_offsets:
            addr=struct.unpack_from("<Q",native,off)[0]
            if addr in reverse:ptr(native,off,reverse[addr])
        assert got==ret,(case,got,ret)
        assert bytes(native)==bufs[0],(case,[(hex(i),a,b) for i,(a,b) in enumerate(zip(native,bufs[0])) if a!=b][:24])
        assert bytes(no)==bytes(m.mu.mem_read(OTHER,SIZE)),(case,"other")
        assert bytes(db)==bytes(m.mu.mem_read(DESC,len(db))),(case,"descriptor")
        assert all(bytes(b)==bytes(m.mu.mem_read(a,len(b))) for a,b in zip(RINGS,rings))
        assert oc==nc and oq==nq and on==nn,(case,oc,nc,oq,nq,on,nn)
        expected=bytes(writes.get(img.base+0x31860+i,bytes([v]))[0] for i,v in enumerate(errors))
        assert bytes(err)==expected,(case,"error")
        assert not m.imports_called,m.imports_called
    print("Original BinkDoFrameAsyncWait PASS: 512 complete handle/queue/completion cases")
if __name__=="__main__":main()
