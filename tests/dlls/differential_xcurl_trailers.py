#!/usr/bin/env python3
"""Execute pinned original 10420 with application/CRT/allocator boundaries.
Compare exact trailer bytes, ownership of request buffers, callback result,
and cancellation code. Intentional bound OOM length clearing is checked
separately, rather than asserted to match the unsafe retail output.
"""
from differential_xcurl_request import *
CB=C.CFUNCTYPE(C.c_int,C.POINTER(P),P)
OLD=A+0x4000;NEW=A+0x5000;NODE=A+0x7000;STR=A+0x8000;OUT=A+0x9000;CB1=A+0xa000;USER=0x12345678
SZ=0x10000
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SZ);lib=C.CDLL(str(library.resolve()))
    fn=lib.bound_append_upload_trailers;fn.argtypes=(P,C.POINTER(Q),C.POINTER(Services));fn.restype=U
    append=lib.curl_slist_append;append.argtypes=(P,C.c_char_p);append.restype=P
    rng=random.Random(0x10420);total=0
    def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
    for i in range(480):
        oc=[];nc=[];cancel=[];diagnostics=[]
        values=[b'X-'+str(j).encode()+b': '+bytes(rng.choice(b'abcdef0123456789 ') for _ in range(rng.randrange(100))) for j in range(i%7)]
        expected=b'0\r\n'+b''.join(v+b'\r\n' for v in values)+b'\r\n' if values else b''
        existing=i%3!=0;cap=[0,16,512,1024][i%4]
        abort=i%11==0;oom=i%13==0 and not abort and bool(values) and (not existing or cap<len(expected))
        state=bytearray(SZ)
        def put(a,v):struct.pack_into('<Q',state,a-A,v)
        put(R+0x30,E);put(R+0xb0,OLD if existing else 0);put(R+0xa8,cap)
        put(E+0x138,USER);put(E+0x140,CB1);state[CB1-A]=0xc3
        for j,v in enumerate(values):
            n=NODE+j*16;s=STR+j*128
            put(n,s);put(n+8,n+16 if j+1<len(values) else 0)
            state[s-A:s-A+len(v)+1]=v+b'\0'
        state[OLD-A:OLD-A+1024]=b'\xa5'*1024
        state[NEW-A:NEW-A+1024]=b'\xa5'*1024
        def callback(mu):
            assert reg(mu,'RDX')==USER
            mu.mem_write(reg(mu,'RCX'),struct.pack('<Q',NODE if values else 0))
            oc.append(('callback',));return int(abort)
        def allocate(mu):
            oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return 0 if oom else NEW
        def release(mu):
            if reg(mu,'EDX')==16:oc.append(('free',reg(mu,'RCX'),16))
            else:assert reg(mu,'EDX')==5
            return 0
        def fmt(mu):
            fmt=m.read_cstr(reg(mu,'R8'))
            assert fmt==b'%hs\r\n',(i,fmt,bytes(mu.mem_read(img.base+0x21b30,16)))
            data=m.read_cstr(reg(mu,'R9'))+b'\r\n'
            assert len(data)<reg(mu,'RDX')
            mu.mem_write(reg(mu,'RCX'),data+b'\0');return len(data)
        def can(mu):
            assert reg(mu,'RCX')==R
            cancel.append((reg(mu,'EDX'),reg(mu,'R8D')));return 0
        def setup(w):
            m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28000,img.base+0x28000)
        ret,_,_,_=m.call(img.base+0x10420,[R,OUT],'uint',setup=setup,
            fakes={CB1:callback,img.base+0x932c:allocate,img.base+0x9640:release,
                   img.base+0x1599c:fmt,img.base+0xa224:lambda mu:0,img.base+0xa32c:lambda mu:0,img.base+0x12f34:can})
        orig_len=struct.unpack('<Q',m.mu.mem_read(OUT,8))[0]
        orig_ptr=struct.unpack('<Q',m.mu.mem_read(R+0xb0,8))[0]
        orig_cap=struct.unpack('<Q',m.mu.mem_read(R+0xa8,8))[0]
        if not ret and orig_len:assert bytes(m.mu.mem_read(orig_ptr,orig_len))==expected
        native=C.create_string_buffer(bytes(state),SZ);base=C.addressof(native)
        def host(v):return base+v-A if A<=v<A+SZ else v
        def norm(v):return A+v-base if v and base<=v<base+SZ else v or 0
        @CB
        def ncb(out,user):
            assert user==USER;nc.append(('callback',))
            lst=None
            for v in values:
                lst=append(lst,v);assert lst
            out[0]=lst;return int(abort)
        for off,v in [(R+0x30,E),(R+0xb0,OLD if existing else 0),(E+0x140,C.cast(ncb,P).value)]:
            struct.pack_into('<Q',native,off-A,host(v))
        @ALLOC
        def na(n,t,c):nc.append(('alloc',n,t));return 0 if oom else host(NEW)
        @FREE
        def nf(p,t,c):nc.append(('free',norm(p),t))
        @DIAG
        def diag(p,k,v,c):diagnostics.append(v)
        lock=LOCK(lambda p,c:None)
        rs=Services(lock,lock,na,nf,LOCK(),LOCK(),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),diag,
                    DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),LOCK(),SUBMIT(),None)
        out=Q(0);status=fn(host(R),C.byref(out),C.byref(rs))
        got_ptr=norm(struct.unpack_from('<Q',native,R-A+0xb0)[0]);got_cap=struct.unpack_from('<Q',native,R-A+0xa8)[0]
        got_cancel=struct.unpack_from('<I',native,E-A+0x228)[0],struct.unpack_from('<I',native,E-A+0x220)[0]
        assert status==ret and (got_ptr,got_cap)==(orig_ptr,orig_cap) and nc==oc,(i,status,ret,nc,oc,got_ptr,orig_ptr,got_cap,orig_cap)
        assert got_cancel==(cancel[0] if cancel else (0,0)),(i,got_cancel,cancel)
        if oom:assert out.value==0 and orig_len==len(expected) # deliberate safety difference
        else:assert out.value==orig_len,(i,out.value,orig_len)
        if not status and out.value:assert C.string_at(host(got_ptr),out.value)==expected
        total+=1
    print('XCurl original trailer formatter PASS:',total,'cases (including explicit OOM guard)',flush=True)
if __name__=='__main__':main()
