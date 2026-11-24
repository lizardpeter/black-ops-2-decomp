#!/usr/bin/env python3
from differential_xcurl_request import *
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));fn=lib.xcurl_research_copy_utf16;fn.argtypes=(P,C.POINTER(P),ALLOC,P);fn.restype=U
    rng=random.Random(0x94ac)
    for i in range(256):
        src=struct.pack('<'+'H'*(i%64+1),*[rng.randrange(1,65536) for _ in range(i%64)],0);initial=b'\xa5'*256;fail=i%3==0;oc=[];nc=[]
        def setup(w):m.mu.mem_write(A,struct.pack('<Q',0x12345678));m.mu.mem_write(BUF,src);m.mu.mem_write(MSG,initial)
        def alloc(mu):oc.append((mu.reg_read(X.UC_X86_REG_RCX),mu.reg_read(X.UC_X86_REG_EDX)));return 0 if fail else MSG
        ret,_,_,_=m.call(img.base+0x94ac,[BUF,0,A],'uint',setup=setup,fakes={img.base+0x932c:alloc});assert isinstance(ret,int),ret
        expected=bytes(m.mu.mem_read(MSG,256));op=struct.unpack('<Q',m.mu.mem_read(A,8))[0];dest=C.create_string_buffer(initial,256);source=C.create_string_buffer(src);out=P(0x12345678)
        @ALLOC
        def na(n,t,c):nc.append((n,t));return None if fail else C.addressof(dest)
        got=fn(C.addressof(source),C.byref(out),na,None)
        assert (got,nc,MSG if out.value else 0,bytes(dest))==(ret,oc,op,expected),(i,got,ret,nc,oc,op,bytes(dest),expected)
    print('XCurl UTF16 copy PASS: 256 cases')
if __name__=='__main__':main()
