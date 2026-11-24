#!/usr/bin/env python3
"""Compare original 0x1AC88 ownership and output against source, with explicit Win32 conversion model."""
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
A=0x34000000;STR=A+0x1000;OLD=A+0x2000;NEW=A+0x3000;CAP=A+0x4000;WR=A+0x4008

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000)
    lib=C.CDLL(str(library.resolve()));fn=lib.xcurl_research_wide_string
    fn.argtypes=(C.c_void_p,C.c_char_p,C.c_uint32,C.POINTER(C.c_uint64),C.POINTER(C.c_uint64),ALLOC,FREE,C.c_void_p);fn.restype=C.c_uint32
    strings=[None,b'',b'hello',b'\xc3\xa9',b'\xe4\xbd\xa0\xe5\xa5\xbd',b'\xf0\x9f\x98\x80',b'a\xf0\x9f\x98\x80z',b'\xff',b'\xc0\x80',b'\xed\xa0\x80',b'\xe2\x82',b'\xf4\x90\x80\x80']
    count=0
    for st,existing,capacity,has_written,fail,tag in itertools.product(strings,[False,True],[None,0,1,3,32],[False,True],[False,True],[5,7]):
        count+=1;oc=[];nc=[];init=b'\xa5'*128
        def oa(mu):
            size=mu.reg_read(X.UC_X86_REG_RCX);t=mu.reg_read(X.UC_X86_REG_EDX);oc.append(('alloc',size,t));return 0 if fail else NEW
        def of(mu):oc.append(('free',mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0
        def convert(mu):
            assert mu.reg_read(X.UC_X86_REG_ECX)==65001 and mu.reg_read(X.UC_X86_REG_EDX)==8
            src=mu.reg_read(X.UC_X86_REG_R8);n=mu.reg_read(X.UC_X86_REG_R9D);sp=mu.reg_read(X.UC_X86_REG_RSP)
            dst,cap=struct.unpack('<QQ',mu.mem_read(sp+0x28,16));data=bytes(mu.mem_read(src,n))
            try:out=data.decode('utf-8','strict').encode('utf-16-le')
            except UnicodeError:out=b''
            units=len(out)//2
            if dst and cap and units:assert cap>=units;mu.mem_write(dst,out)
            mu.reg_write(X.UC_X86_REG_RAX,units)
        for name in img.iat.values():
            if name.endswith('!MultiByteToWideChar'):m.on_import(name,convert)
        def setup(w):
            m.mu.mem_write(A,struct.pack('<Q',OLD if existing else 0));m.mu.mem_write(STR,(st or b'')+b'\0');m.mu.mem_write(OLD,init);m.mu.mem_write(NEW,init)
            m.mu.mem_write(CAP,struct.pack('<QQ',capacity or 0,0x87654321))
        ret,_,_,_=m.call(img.base+0x1ac88,[A,STR if st is not None else 0,tag,CAP if capacity is not None else 0,WR if has_written else 0],'uint',setup=setup,fakes={img.base+0x932c:oa,img.base+0x9640:of})
        assert isinstance(ret,int),(count,ret,m.imports_called)
        original_ptr=struct.unpack('<Q',m.mu.mem_read(A,8))[0];original_cap,original_wr=struct.unpack('<QQ',m.mu.mem_read(CAP,16))
        old=C.create_string_buffer(init,128);new=C.create_string_buffer(init,128);op=C.addressof(old);np=C.addressof(new)
        slot=C.c_void_p(op if existing else 0);cap=C.c_uint64(capacity or 0);wr=C.c_uint64(0x87654321)
        @ALLOC
        def na(size,t,c):nc.append(('alloc',size,t));return None if fail else np
        @FREE
        def nf(ptr,t,c):nc.append(('free',OLD if ptr==op else NEW if ptr==np else ptr,t))
        got=fn(C.byref(slot),st,tag,C.byref(cap) if capacity is not None else None,C.byref(wr) if has_written else None,na,nf,None)
        ptr=OLD if slot.value==op else NEW if slot.value==np else slot.value or 0
        assert (got,nc,ptr,cap.value,wr.value,bytes(old),bytes(new))==(ret,oc,original_ptr,original_cap,original_wr,bytes(m.mu.mem_read(OLD,128)),bytes(m.mu.mem_read(NEW,128))),(st,existing,capacity,has_written,fail,tag,got,ret,nc,oc,ptr,original_ptr,cap.value,original_cap,wr.value,original_wr)
    print(f'XCurl wide-string PASS: {count} UTF-8/ownership/capacity/failure cases')
if __name__=='__main__':main()
