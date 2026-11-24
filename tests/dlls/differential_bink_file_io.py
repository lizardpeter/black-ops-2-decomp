#!/usr/bin/env python3
"""Direct original file IO state/Win32 read differential. No codec mocks."""
import ctypes
import hashlib
import random
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/"tests/differential"))
from harness import Image,Machine,HEAP
from differential_bink_resident_state import PIN,put,ptr
from unicorn import x86_const as X
IO_SIZE=0x110
ARENA=0x34000000
ARENA_SIZE=0x80000
SEEK=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint64)
READ=ctypes.CFUNCTYPE(ctypes.c_uint32,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32)
FILL=ctypes.CFUNCTYPE(ctypes.c_int64,ctypes.c_void_p,ctypes.c_void_p)
TRY=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_void_p)
LOCK=ctypes.CFUNCTYPE(None,ctypes.c_void_p)
TIMER=ctypes.CFUNCTYPE(ctypes.c_uint32)
def get32(h,off):return struct.unpack_from("<I",h,off)[0]
def get64(h,off):return struct.unpack_from("<Q",h,off)[0]

def main():
    dll,source=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==PIN
    img=Image(dll);m=Machine(img)
    m.mu.mem_map(ARENA,ARENA_SIZE)
    lib=ctypes.CDLL(str(source.resolve()))
    size=lib.bink_research_file_io_reserve
    size.argtypes=(ctypes.c_uint32,);size.restype=ctypes.c_uint32
    buf=lib.bink_research_file_set_buffer
    buf.argtypes=(ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32);buf.restype=ctypes.c_int
    header=lib.bink_research_file_read_header
    header.argtypes=(ctypes.c_void_p,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,SEEK,READ,ctypes.c_void_p);header.restype=ctypes.c_int64
    refill=lib.bink_research_file_refill
    refill.argtypes=(ctypes.c_void_p,READ,ctypes.c_void_p);refill.restype=ctypes.c_int64
    control=lib.bink_research_file_control
    control.argtypes=(ctypes.c_void_p,ctypes.c_uint32,FILL,ctypes.c_void_p);control.restype=ctypes.c_uint32
    rng=random.Random(0x20990)
    for n in [0,1,0x1ffff,0x20000,0x20001,0x3ffff,0x40000,0xffffffff]+[rng.getrandbits(32) for _ in range(256)]:
        r,_,_,_=m.call(img.base+0x20990,[0,n],"uint")
        assert size(n)==r,(n,size(n),r)
    print("Original file I/O reserve PASS: 264 cases")
    # Keep matching low address bits, compare normalized buffer pointers.
    native_mem=(ctypes.c_uint8*(ARENA_SIZE+0x1000))()
    native_base=(ctypes.addressof(native_mem)+0xfff)&~0xfff
    for i in range(128):
        h=bytearray(rng.randbytes(IO_SIZE))
        ptr(h,0x40,0);ptr(h,0x50,0)
        start=rng.randrange(0x1000)
        n=rng.randrange(0x21000,0x70000)
        end=rng.getrandbits(32)
        r,bs,_,_=m.call(img.base+0x209b0,[bytes(h),ARENA+start,n,end],"void")
        assert r is None
        nh=(ctypes.c_uint8*IO_SIZE).from_buffer(h)
        assert buf(nh,native_base+start,n,end)==0
        for off in (0xD8,0xE0,0x100):ptr(h,off,get64(h,off)-native_base+ARENA)
        assert bytes(h)==bs[0],(i,"buffer",[(hex(j),a,b) for j,(a,b) in enumerate(zip(h,bs[0])) if a!=b][:20])
    print("Original file I/O buffer initialization PASS: 128 full states")
    # Header read with file base, seek/no seek, full and short reads.
    for i in range(192):
        h=bytearray(rng.randbytes(IO_SIZE))
        for off in (0x40,0x50,0x70):ptr(h,off,0)
        base=rng.randrange(0,1<<40)
        ptr(h,0xC0,base);ptr(h,0xC8,0x76543210)
        pos=rng.randrange(0,10000)
        n=rng.randrange(1,512)
        got=n if i%3 else rng.randrange(n)
        offset=[0xffffffff,pos,rng.randrange(0,10000)][i%3]
        put(h,0xF4,pos);put(h,0xEC,20000);put(h,0xA0,4096)
        payload=rng.randbytes(got)
        initial=rng.randbytes(528)
        os=[];ns=[]
        def seek_win(mu):
            low=mu.reg_read(X.UC_X86_REG_EDX);high=mu.reg_read(X.UC_X86_REG_R8)
            target=low|(struct.unpack("<I",bytes(mu.mem_read(high,4)))[0]<<32)
            assert mu.reg_read(X.UC_X86_REG_RCX)==0x76543210
            assert mu.reg_read(X.UC_X86_REG_R9D)==0
            os.append(("seek",target));mu.reg_write(X.UC_X86_REG_RAX,low)
        def read_win(mu):
            assert mu.reg_read(X.UC_X86_REG_RCX)==0x76543210
            count=mu.reg_read(X.UC_X86_REG_R8D)
            os.append(("read",count))
            assert count==n
            if got:mu.mem_write(mu.reg_read(X.UC_X86_REG_RDX),payload)
            mu.mem_write(mu.reg_read(X.UC_X86_REG_R9),struct.pack("<I",got))
            mu.reg_write(X.UC_X86_REG_RAX,1)
        m.on_import("kernel32.dll!SetFilePointer",seek_win)
        m.on_import("kernel32.dll!ReadFile",read_win)
        r,bs,_,_=m.call(img.base+0x200c0,[bytes(h),offset,initial,n],"uint")
        @SEEK
        def seek_native(ctx,target):ns.append(("seek",target))
        @READ
        def read_native(ctx,dest,count):
            ns.append(("read",count));ctypes.memmove(dest,payload,got);return got
        nh=(ctypes.c_uint8*IO_SIZE).from_buffer(h)
        out=(ctypes.c_uint8*len(initial)).from_buffer_copy(initial)
        actual=header(nh,offset,out,n,seek_native,read_native,None)
        assert actual==r and os==ns and bytes(out)==bs[1],(i,actual,r,os,ns)
        assert bytes(h)==bs[0],(i,"header",[(hex(j),a,b) for j,(a,b) in enumerate(zip(h,bs[0])) if a!=b][:20])
    print("Original file I/O header read PASS: 192 states/read buffers")
    # Background control callback dispatch and exact state updates.
    for i in range(192):
        h=bytearray(rng.randbytes(IO_SIZE));put(h,0xB0,i%4)
        ptr(h,0x40,0);ptr(h,0x50,0)
        flags=rng.choice([0,1,2,3,0x80000000,0x80000001,0x80000002,0x80000003])
        original=[];native=[]
        def ofill(mu):original.append(1);return 0
        @FILL
        def nfill(io,ctx):native.append(1);return 0
        r,bs,_,_=m.call(img.base+0x20f80,[bytes(h),flags],"uint",fakes={img.base+0x20aa0:ofill})
        nh=(ctypes.c_uint8*IO_SIZE).from_buffer(h)
        assert control(nh,flags,nfill,None)==r and original==native and bytes(h)==bs[0],(i,flags)
    print("Original background file control PASS: 192 full states")
    # Refill one actual 128KiB ring sector, including first-sector alignment
    # skip, final short sector and OS short-read result.
    fake_try=img.base+0x17950
    fake_timer=img.base+0x1ff00
    fake_unlock=img.base+0x17970
    for i in range(96):
        h=bytearray(rng.randbytes(IO_SIZE))
        for off in (0x40,0x58,0x60,0x70,0x78):ptr(h,off,0)
        ptr(h,0x48,fake_try);ptr(h,0x50,fake_unlock);ptr(h,0x68,fake_timer)
        ptr(h,0xC8,0x76543210)
        cursor=ARENA+(i%2)*0x20000
        ptr(h,0xD8,ARENA);ptr(h,0xE0,ARENA+0x40000);ptr(h,0x100,cursor)
        pos=0x20000+(i%3)*31
        remaining=[0x30000,0x10000,0x40000][i%3]
        count=min(0x20000,remaining)
        skip=pos&0x1ffff
        if skip>count or count==remaining:skip=0
        count-=skip
        got=count if i%4 else count//2
        for off,value in ((0x80,0),(0x84,0),(0xB0,0),(0x108,0),(0xF4,pos),(0xEC,pos+remaining),
            (0xF8,0x40000),(0xA0,0x40000),(0xAC,0),(0xA4,0),(0xF0,0),(0x8C,i%2)):
            put(h,off,value)
        payload=rng.randbytes(got)
        initial=bytes([0xC9])*0x40000
        m.mu.mem_write(ARENA,initial)
        ctypes.memmove(native_base,initial,len(initial))
        oc=[];nc=[];os=[];ns=[]
        def otry(mu):oc.append("try");return 1
        def ounlock(mu):oc.append("unlock");return 0
        def otimer(mu):oc.append("timer");return 1000+len(oc)
        @TRY
        def ntry(io):nc.append("try");return 1
        @LOCK
        def nunlock(io):nc.append("unlock")
        @TIMER
        def ntimer():nc.append("timer");return 1000+len(nc)
        def read_win(mu):
            requested=mu.reg_read(X.UC_X86_REG_R8D)
            dest=mu.reg_read(X.UC_X86_REG_RDX)
            os.append((dest-ARENA,requested))
            assert requested==count,(i,requested,count)
            mu.mem_write(dest,payload)
            mu.mem_write(mu.reg_read(X.UC_X86_REG_R9),struct.pack("<I",got))
            mu.reg_write(X.UC_X86_REG_RAX,1)
        m.on_import("kernel32.dll!ReadFile",read_win)
        r,bs,_,_=m.call(img.base+0x20aa0,[bytes(h)],"uint",
            fakes={fake_try:otry,fake_unlock:ounlock,fake_timer:otimer})
        assert isinstance(r,int),(i,r)
        for off,cb in ((0x48,ntry),(0x50,nunlock),(0x68,ntimer)):ptr(h,off,ctypes.cast(cb,ctypes.c_void_p).value)
        for off in (0xD8,0xE0,0x100):ptr(h,off,get64(h,off)-ARENA+native_base)
        @READ
        def native_read(ctx,dest,requested):
            ns.append((dest-native_base,requested));ctypes.memmove(dest,payload,got);return got
        nh=(ctypes.c_uint8*IO_SIZE).from_buffer(h)
        actual=refill(nh,native_read,None)
        for off,val in ((0x48,fake_try),(0x50,fake_unlock),(0x68,fake_timer)):ptr(h,off,val)
        for off in (0xD8,0xE0,0x100):ptr(h,off,get64(h,off)-native_base+ARENA)
        assert actual==r and oc==nc and os==ns,(i,actual,r,oc,nc,os,ns)
        assert bytes(h)==bs[0],(i,"refill",[(hex(j),a,b) for j,(a,b) in enumerate(zip(h,bs[0])) if a!=b][:24])
        assert ctypes.string_at(native_base,len(initial))==bytes(m.mu.mem_read(ARENA,len(initial))),(i,"buffer")
    print("Original sequential ring refill PASS: 96 state/buffer cases")
if __name__=="__main__":main()
