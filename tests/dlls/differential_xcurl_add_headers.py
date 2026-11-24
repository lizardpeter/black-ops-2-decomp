#!/usr/bin/env python3
"""Original request header list, buffer reuse, case-sensitive chunk detection and error mapping."""
from differential_xcurl_request import *
ADD=C.CFUNCTYPE(U,P,P,U,U,P)
class AddHeaders(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('add',ADD)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x10038);fn=lib.xcurl_research_add_request_headers;fn.argtypes=(P,P,C.POINTER(AddHeaders));fn.restype=U
    NODE=A+0x3000;TEXT=A+0x4000;WIDE=A+0x5000
    for i in range(1024):
        state=bytearray(SIZE);oc=[];nc=[];offsets=set();oi=ni=oa=na=0;last_o=last_n=0
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        q(R+0x30,E);q(R+0x50,HTTP);u(R+0xd0,rng.randrange(4));u(E+0x80,rng.randrange(4));q(E+0xc0,0x12345000 if i%2 else 0);state[E-A+0x176]=i%2
        lines=[rng.choice([b'X-Test: a',b'Transfer-Encoding: chunked',b'transfer-encoding: chunked',b'Transfer-Encoding: Chunked',b'Transfer-Encoding: gzip, chunked',b'Transfer-Encoding',b'',b'X-Unicode: \xc3\xa9',b'X-Invalid: \xff']) for _ in range(i%5)]
        for j,t in enumerate(lines):
            q(NODE+j*16,TEXT+j*128);q(NODE+j*16+8,NODE+(j+1)*16 if j+1<len(lines) else 0);state[TEXT-A+j*128:TEXT-A+j*128+len(t)+1]=t+b'\0'
        state[WIDE-A:WIDE-A+0x400]=b'\xa5'*0x400;fail=rng.randrange(6);results=[rng.choice([0,1,1,1]) for _ in lines];errors=[rng.choice([0,5,12150,12150,0x80004005]) for _ in lines]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def alloc(mu):
            nonlocal oa
            n,t=reg(mu,'RCX'),reg(mu,'EDX');oc.append(('alloc',n,t));oa+=1;return 0 if oa==fail else WIDE
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def conv(mu):
            assert reg(mu,'ECX')==65001 and reg(mu,'EDX')==8;src,n=reg(mu,'R8'),reg(mu,'R9D');d,cap=struct.unpack('<QQ',mu.mem_read(reg(mu,'RSP')+0x28,16));raw=bytes(mu.mem_read(src,n));
            try:out=raw.decode('utf-8','strict').encode('utf-16le')
            except UnicodeError:out=b''
            if d and cap and out:mu.mem_write(d,out)
            mu.reg_write(X.UC_X86_REG_RAX,len(out)//2)
        def wcsncmp(mu):
            a,b,n=reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8');x=bytes(mu.mem_read(a,n*2));y=bytes(mu.mem_read(b,n*2));mu.reg_write(X.UC_X86_REG_RAX,(int(x>y)-int(x<y))&0xffffffff)
        def add(mu):
            nonlocal oi,last_o
            h,p,n,f=[reg(mu,k) for k in ('RCX','RDX','R8D','R9D')];oc.append(('add',h,p,n,f,bytes(mu.mem_read(p,n*2))));result=results[oi];last_o=errors[oi];oi+=1;mu.reg_write(X.UC_X86_REG_RAX,result)
        def err(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,last_o)
        def dbg(mu):
            e,k,p,n=[reg(mu,x) for x in ('RCX','EDX','R8','R9')];oc.append(('debug',e,k,p,n,bytes(mu.mem_read(p,n))));return 0
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base,reg(mu,'R8D')));return 0
        for name in img.iat.values():
            f={'MultiByteToWideChar':conv,'wcsncmp':wcsncmp,'WinHttpAddRequestHeaders':add,'GetLastError':err}.get(name.split('!')[-1]);
            if f:m.on_import(name,f)
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+0x10038,[R,NODE if lines else 0],'uint',setup=setup,fakes={img.base+0x932c:alloc,img.base+0x9640:free,img.base+0xa128:dbg,img.base+0xa224:diag});assert isinstance(ret,int),(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        @ALLOC
        def al(n,t,c):
            nonlocal na
            nc.append(('alloc',n,t));na+=1;return None if na==fail else host(WIDE)
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @ADD
        def ad(h,p,n,f,c):
            nonlocal ni,last_n
            nc.append(('add',norm(h),norm(p),n,f,C.string_at(p,n*2)));v=results[ni];last_n=errors[ni];ni+=1;return v
        @ERROR
        def er(c):nc.append(('error',));return last_n
        @DIAG
        def di(e,k,v,c):nc.append(('diag',norm(e),k,v))
        @DEBUG
        def db(e,k,p,n,c):nc.append(('debug',norm(e),k,norm(p),n,C.string_at(p,n)))
        rs=Services(LOCK(),LOCK(),al,fr,LOCK(),LOCK(),LOCK(),ADJUST(),EVENT(),er,QUEUE(),di,DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),db,LOCK(),SUBMIT(),None)
        svc=AddHeaders(C.pointer(rs),ad);got=fn(host(R),host(NODE) if lines else 0,C.byref(svc));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(ret,oc),(i,lines,got,ret,nc,oc)
        assert raw==expected,(i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
    print('XCurl original outgoing header list PASS: 1024 cases')
if __name__=='__main__':main()
