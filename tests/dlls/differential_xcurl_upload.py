#!/usr/bin/env python3
"""Original streamed upload, chunk framing and application callback trampoline."""
from differential_xcurl_request import *
CB=C.CFUNCTYPE(Q,P,Q,Q,P);WRITE=C.CFUNCTYPE(U,P,P,U,P);TRAILERS=C.CFUNCTYPE(U,P,C.POINTER(Q),P);UD=C.CFUNCTYPE(None,P,U,Q,Q,P)
class Upload(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('write',WRITE),('trailers',TRAILERS),('diag',UD)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x12868);total=0
    B=A+0x4000;CB1=A+0x5900;USER=0x12345678
    for name,rva in [('upload_continue',0x12868),('upload_worker',0xc9c0)]:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(P,C.POINTER(Upload));fn.restype=None
      for i in range(512):
        total+=1;state=bytearray(SIZE);oc=[];nc=[];offsets=set();capacity=rng.choice([0,1,15,16,255,256,512]);existing=i%3!=0;alloc_ok=i%7!=0;mode=rng.randrange(4);active=i%5!=0;has_cb=i%11!=0;trailer=i%4==0;trail_status=rng.choice([0,1,0x80004005]);trail_bytes=rng.randrange(10);result=rng.choice([0,capacity,capacity//2,capacity+1,0x10000000,0x10000001,0xffffffffffffffff]);write_ok=i%6!=0;err=rng.choice([0,5,12002,0xffffffff,0x80004005]);payload=rng.randbytes(capacity)
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        q(R+0x30,E);q(R+0x50,HTTP);q(R+0xb0,B if existing else 0);q(R+0xa8,capacity+12);u(R+0x28,2 if active else 5);u(R+0xd0,mode);state[R-A+0xd5]=rng.randrange(2);sent=rng.getrandbits(64);q(R+0xa0,sent)
        q(E+0x120,CB1 if has_cb else 0);q(E+0x118,USER);q(E+0x140,USER if trailer else 0);u(E+0xb0,capacity);u(E+0x80,rng.randrange(4));q(E+0xf0,rng.choice([0,0xffffffffffffffff,(sent+result)&0xffffffffffffffff]));q(E+0xf8,rng.choice([0,0xffffffffffffffff,(sent+result)&0xffffffffffffffff]));u(E+0x228,rng.choice([0,1,0x80004004]));u(E+0x220,77);state[E-A+0x176]=rng.randrange(2);state[B-A:B-A+1024]=b'\xa5'*1024;state[CB1-A]=0xc3
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def alloc(mu):oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return B if alloc_ok else 0
        def cb(mu):
            p,size,n,ud=[reg(mu,k) for k in ('RCX','RDX','R8','R9')];oc.append(('read_cb',p,size,n,ud));mu.mem_write(p,payload);return result
        def write(mu):
            h,p,n,z=[reg(mu,k) for k in ('RCX','RDX','R8D','R9')];assert z==0;oc.append(('write',h,p,n,bytes(mu.mem_read(p,n)) if p else b''));mu.reg_write(X.UC_X86_REG_RAX,int(write_ok))
        def error(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,err)
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x20a08 else reg(mu,'R8') if site==0x21378 else reg(mu,'R8D'),reg(mu,'R9') if site==0x21378 else 0));return 0
        def debug(mu):
            e,k,p,n=[reg(mu,x) for x in ('RCX','EDX','R8','R9')];oc.append(('debug',e,k,p,n,bytes(mu.mem_read(p,n))));return 0
        def trailers(mu):
            assert reg(mu,'RCX')==R;oc.append(('trailers',R));mu.mem_write(reg(mu,'RDX'),struct.pack('<Q',trail_bytes));mu.mem_write(B,b'T'*trail_bytes);return trail_status
        def format_chunk(mu):
            assert reg(mu,'RDX')==11 and m.read_cstr(reg(mu,'R8'))==b'%zx%s';p=reg(mu,'RCX');n=reg(mu,'R9');suffix=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];assert m.read_cstr(suffix)==b'\r\n';data=f'{n:x}\r\n'.encode();mu.mem_write(p,data+b'\0');return len(data)
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'WinHttpCloseHandle':lambda mu:log(mu,'close'),'WinHttpWriteData':write,'GetLastError':error}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+rva,[R if name=='upload_continue' else E,0],'void',setup=setup,fakes={img.base+0x932c:alloc,CB1:cb,img.base+0xa224:diag,img.base+0xa128:debug,img.base+0x10420:trailers,img.base+0x1599c:format_chunk});assert ret is None,(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        @CB
        def ncb(p,s,n,u):nc.append(('read_cb',norm(p),s,n,norm(u)));C.memmove(p,payload,len(payload));return result
        ncb_ptr=C.cast(ncb,P).value
        for off in offsets:
            v=struct.unpack_from('<Q',state,off)[0];struct.pack_into('<Q',native,off,ncb_ptr if v==CB1 else host(v))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return host(B) if alloc_ok else None
        @ERROR
        def er(c):nc.append(('error',));return err
        @WRITE
        def wr(h,p,n,c):nc.append(('write',norm(h),norm(p),n,C.string_at(p,n) if p else b''));return int(write_ok)
        @UD
        def di(e,k,a,b,c):nc.append(('diag',norm(e),k,a,b))
        @DEBUG
        def db(e,k,p,n,c):nc.append(('debug',norm(e),k,norm(p),n,C.string_at(p,n)))
        @TRAILERS
        def tr(p,n,c):nc.append(('trailers',norm(p)));n[0]=trail_bytes;C.memmove(host(B),b'T'*trail_bytes,trail_bytes);return trail_status
        rs=Services(lock('enter'),lock('leave'),al,FREE(),LOCK(),lock('close'),LOCK(),ADJUST(),EVENT(),er,QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),db,LOCK(),SUBMIT(),None)
        us=Upload(C.pointer(rs),wr,tr,di);fn(host(R if name=='upload_continue' else E),C.byref(us));raw=bytearray(bytes(native))
        for off in offsets:
            v=struct.unpack_from('<Q',raw,off)[0];struct.pack_into('<Q',raw,off,CB1 if v==ncb_ptr else norm(v))
        assert nc==oc,(name,i,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 512',flush=True)
    print('XCurl original streamed upload PASS:',total,'cases')
if __name__=='__main__':main()
