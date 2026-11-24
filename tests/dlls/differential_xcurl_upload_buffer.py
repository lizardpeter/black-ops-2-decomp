#!/usr/bin/env python3
"""Original fixed/buffered upload allocation and callback loop (safe original input domain)."""
from differential_xcurl_request import *
CB=C.CFUNCTYPE(Q,P,Q,Q,P);REALLOC=C.CFUNCTYPE(P,P,C.c_size_t,U,P);UD=C.CFUNCTYPE(None,P,U,Q,Q,P)
class Buffer(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('realloc',REALLOC),('diag',UD)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0xc070);total=0
    B=A+0x4000;CB1=A+0x5900;USER=0x12345678;OUT=A+0x80;CAP=A+0x88;LEN=A+0x90
    for name,rva,argspec in [('allocate_upload',0x102fc,(P,C.POINTER(P),Q)),('buffer_upload',0xc070,(P,C.POINTER(P),C.POINTER(Q),C.POINTER(Q)))]:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*argspec,C.POINTER(Buffer));fn.restype=U
      for i in range(512):
        total+=1;state=bytearray(SIZE);oc=[];nc=[];block=rng.choice([1,16,64,256]);size=rng.choice([0,1,0xffffffffffffffff]);post=rng.choice([0,1,0xffffffffffffffff]);has_cb=i%7!=0;alloc_ok=i%5!=0;realloc_ok=i%6!=0;seq=rng.choice([[0],[block,0],[block//2,block-block//2,0],[0x10000000],[block,0x10000001]]);oi=ni=0;payload=rng.randbytes(block)
        def q(a,v):struct.pack_into('<Q',state,a-A,v)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        q(R+0x30,E);q(R+0x50,HTTP);u(R+0x28,i%7);q(E+0x120,CB1 if has_cb else 0);q(E+0x118,USER);u(E+0xb0,block);q(E+0xf0,size);q(E+0xf8,post);u(E+0x228,rng.choice([0,1,0x80004004]));u(E+0x220,77);q(OUT,0x77777777);q(CAP,1234);q(LEN,5678);state[B-A:B-A+1024]=b'\xa5'*1024;state[CB1-A]=0xc3
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def alloc(mu):oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return B if alloc_ok else 0
        def realloc(mu):oc.append(('realloc',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));return B if realloc_ok else 0
        def cb(mu):
            nonlocal oi
            p,s,n,ud=[reg(mu,k) for k in ('RCX','RDX','R8','R9')];oc.append(('read_cb',p,s,n,ud));v=seq[oi];oi+=1
            if v<=block:mu.mem_write(p,payload[:v])
            return v
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x20a08 else reg(mu,'R8'),reg(mu,'R9') if site==0x209c0 else 0));return 0
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'WinHttpCloseHandle':lambda mu:log(mu,'close')}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        def setup(w):m.mu.mem_write(A,bytes(state))
        args=[R,OUT,block] if name=='allocate_upload' else [E,OUT,CAP,LEN]
        ret,_,_,_=m.call(img.base+rva,args,'uint',setup=setup,fakes={img.base+0x932c:alloc,img.base+0x96f4:realloc,CB1:cb,img.base+0xa224:diag,img.base+0x9640:free,img.base+0x5424:lambda mu:log(mu,'cancel')});assert isinstance(ret,int),(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        @CB
        def ncb(p,s,n,ud):
            nonlocal ni
            nc.append(('read_cb',norm(p),s,n,norm(ud)));v=seq[ni];ni+=1
            if v<=block:C.memmove(p,payload[:v],v)
            return v
        struct.pack_into('<Q',native,R-A+0x30,host(E));struct.pack_into('<Q',native,E-A+0x120,C.cast(ncb,P).value if has_cb else 0)
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return host(B) if alloc_ok else None
        @REALLOC
        def re(p,n,t,c):nc.append(('realloc',norm(p),n,t));return host(B) if realloc_ok else None
        @UD
        def di(e,k,a,b,c):nc.append(('diag',norm(e),k,a,b))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        rs=Services(lock('enter'),lock('leave'),al,fr,LOCK(),lock('close'),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),lock('cancel'),SUBMIT(),None)
        bs=Buffer(C.pointer(rs),re,di);p=C.cast(host(OUT),C.POINTER(P));got=fn(host(R),p,block,C.byref(bs)) if name=='allocate_upload' else fn(host(E),p,C.cast(host(CAP),C.POINTER(Q)),C.cast(host(LEN),C.POINTER(Q)),C.byref(bs));raw=bytearray(bytes(native))
        struct.pack_into('<Q',raw,R-A+0x30,E);struct.pack_into('<Q',raw,E-A+0x120,CB1 if has_cb else 0);struct.pack_into('<Q',raw,OUT-A,norm(p[0]))
        assert (got,nc)==(ret,oc),(name,i,got,ret,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 512',flush=True)
    print('XCurl original upload buffering PASS:',total,'cases')
if __name__=='__main__':main()
