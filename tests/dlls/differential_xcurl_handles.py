#!/usr/bin/env python3
"""Compare handle allocation, default object bytes, refcount, cleanup and option destruction.
The OS critical-section initializer and the still-unported engine start/destroy calls are boundaries.
"""
import ctypes as C
import hashlib,random,struct,sys,itertools
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/differential'))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from unicorn import x86_const as X
ALLOC=C.CFUNCTYPE(C.c_void_p,C.c_size_t,C.c_uint32,C.c_void_p)
FREE=C.CFUNCTYPE(None,C.c_void_p,C.c_uint32,C.c_void_p)
LOCK=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
GLOBAL=C.CFUNCTYPE(C.c_uint32,C.c_void_p)
START=C.CFUNCTYPE(C.c_uint32,C.c_void_p,C.c_void_p)
DESTROY=C.CFUNCTYPE(None,C.c_void_p,C.c_void_p)
class Services(C.Structure):
    _fields_=[('alloc',ALLOC),('free',FREE),('lock',LOCK),('global_start',GLOBAL),('easy_start',START),('easy_destroy',DESTROY),('multi_start',START),('multi_destroy',START),('ctx',C.c_void_p)]
A=0x34000000;H=A+0x1000;OLD=0x12345678

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000)
    lib=C.CDLL(str(library.resolve()))
    ec=lib.xcurl_research_easy_create;ec.argtypes=(C.POINTER(C.c_void_p),C.POINTER(C.c_uint32),C.POINTER(Services));ec.restype=C.c_uint32
    mc=lib.xcurl_research_multi_create;mc.argtypes=(C.POINTER(C.c_void_p),C.POINTER(Services));mc.restype=C.c_uint32
    ed=lib.xcurl_research_easy_destroy;ed.argtypes=(C.c_void_p,C.POINTER(Services));ed.restype=None
    md=lib.xcurl_research_multi_destroy;md.argtypes=(C.c_void_p,C.POINTER(Services));md.restype=C.c_uint32
    od=lib.xcurl_research_options_destroy;od.argtypes=(C.c_void_p,FREE,C.c_void_p);od.restype=None
    rng=random.Random(0xdc94);count=0
    for action in ('easy_create','multi_create','easy_destroy','multi_destroy','options_destroy'):
      for j in range(128):
        count+=1;oc=[];nc=[];refs=[0,1,3,0xffffffff][j%4];global_status=[0,2,27,0][(j//4)%4];start_status=[0,1,0x80004005,0xffffffff][(j//16)%4];fail=j%5==0;present=j%2==0
        init=bytearray(rng.randbytes(0x368));opts=action=='options_destroy'
        if opts:
            init[0x100]=j%3
            for k,off in enumerate((0x10,0x48,0x50,0x58,0x60,0x70)):
                struct.pack_into('<Q',init,off,0x34560000+k*0x100 if rng.randrange(2) else 0)
        def oa(mu):oc.append(('alloc',mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0 if fail else H
        def of(mu):oc.append(('free',mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
        def ol(mu):
            address=mu.reg_read(X.UC_X86_REG_RCX);assert mu.reg_read(X.UC_X86_REG_EDX)==0 and mu.reg_read(X.UC_X86_REG_R8D)==0
            oc.append(('lock',address-H));mu.mem_write(address,b'\x4c'*40);mu.reg_write(X.UC_X86_REG_RAX,1)
        def og(mu):oc.append(('global',));return global_status
        def os(mu,kind):assert mu.reg_read(X.UC_X86_REG_RCX)==H;oc.append((kind,));return start_status
        for name in img.iat.values():
            if name.endswith('!InitializeCriticalSectionEx'):m.on_import(name,ol)
        def setup(w):
            w.u32(img.base+0x28c18,refs);m.mu.mem_write(A,struct.pack('<Q',OLD));m.mu.mem_write(H,bytes(init))
        rv={'easy_create':0xdc94,'multi_create':0x199a8,'easy_destroy':0xdbc0,'multi_destroy':0x19818,'options_destroy':0xe818}[action]
        arg=A if action.endswith('create') else H if opts or present else 0
        ret,_,writes,_=m.call(img.base+rv,[arg],'void' if action in ('easy_destroy','options_destroy') else 'uint',setup=setup,
          fakes={img.base+0x932c:oa,img.base+0x9640:of,img.base+0x9ba0:og,img.base+0xb55c:lambda mu:os(mu,'easy_start'),img.base+0xa544:lambda mu:os(mu,'easy_destroy'),img.base+0x184a8:lambda mu:os(mu,'multi_start'),img.base+0x18110:lambda mu:os(mu,'multi_destroy')})
        assert ret is None or isinstance(ret,int),(action,j,ret,m.imports_called)
        refbytes=struct.pack('<I',refs);oref=struct.unpack('<I',b''.join(writes.get(img.base+0x28c18+i,bytes([v])) for i,v in enumerate(refbytes)))[0]
        native=C.create_string_buffer(bytes(init),len(init));base=C.addressof(native);out=C.c_void_p(OLD);ref=C.c_uint32(refs)
        @ALLOC
        def na(size,tag,c):nc.append(('alloc',size,tag));return None if fail else base
        @FREE
        def nf(ptr,tag,c):nc.append(('free',H if ptr==base else ptr,tag))
        @LOCK
        def nl(ptr,c):nc.append(('lock',ptr-base));C.memset(ptr,0x4c,40)
        @GLOBAL
        def ng(c):nc.append(('global',));return global_status
        def start(kind,ptr):assert ptr==base;nc.append((kind,));return start_status
        @START
        def es(ptr,c):return start('easy_start',ptr)
        @DESTROY
        def ds(ptr,c):start('easy_destroy',ptr)
        @START
        def ms(ptr,c):return start('multi_start',ptr)
        @START
        def dm(ptr,c):return start('multi_destroy',ptr)
        services=Services(na,nf,nl,ng,es,ds,ms,dm,None)
        if action=='easy_create':got=ec(C.byref(out),C.byref(ref),C.byref(services))
        elif action=='multi_create':got=mc(C.byref(out),C.byref(services))
        elif action=='easy_destroy':got=ed(base if present else None,C.byref(services))
        elif action=='multi_destroy':got=md(base if present else None,C.byref(services))
        else:got=od(base,nf,None)
        native_bytes=bytearray(bytes(native))
        for off in range(0,len(native_bytes),8):
            v=struct.unpack_from('<Q',native_bytes,off)[0]
            if base<=v<base+len(init):struct.pack_into('<Q',native_bytes,off,H+v-base)
        op=struct.unpack('<Q',m.mu.mem_read(A,8))[0];np=H if out.value==base else out.value or 0
        assert got==ret and oc==nc and op==np and oref==ref.value,(action,j,got,ret,nc,oc,op,np,oref,ref.value)
        original=bytes(m.mu.mem_read(H,len(init)))
        assert native_bytes==original,(action,j,[(hex(i),a,b) for i,(a,b) in enumerate(zip(native_bytes,original)) if a!=b][:50])
    print(f'XCurl handles PASS: {count} allocation/defaults/ownership/refcount/cleanup cases')
if __name__=='__main__':main()
