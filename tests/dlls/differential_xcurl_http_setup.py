#!/usr/bin/env python3
"""Original connection/open retry and option application, including native WinHTTP ABI arguments."""
from differential_xcurl_request import *
CONNECT=C.CFUNCTYPE(P,P,P,C.c_uint16,P);OPEN=C.CFUNCTYPE(P,P,P,P,P,U,P)
OPTION=C.CFUNCTYPE(U,P,U,U,P);TIMEOUT=C.CFUNCTYPE(U,P,U,U,U,U,P);RETRY=C.CFUNCTYPE(None,P,P)
ACQUIRE=C.CFUNCTYPE(U,P,P,U,U,C.POINTER(P),P)
class Setup(C.Structure):
    _fields_=[('connect',CONNECT),('open',OPEN),('option',OPTION),('timeouts',TIMEOUT),('error',ERROR),('diag',DIAG),('retry',RETRY),('ctx',P)]
URL=A+0x3000;HOST=A+0x3200;PATH=A+0x3300;METHOD=A+0x3400;ACCEPT=A+0x3500;SESSION=A+0x3600;OPT=A+0x4000;ENGINE=A+0x4400;META=A+0x4600
SESSION_HANDLE=0x11110000;CONN=0x11120000;OPEN_HANDLE=0x11130000

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x10748);total=0
    for name,rva,types in [('http_open',0x10748,(P,P,P,P,P,C.c_uint16)),('http_options',0x131a8,(P,P,U)),('http_open_with_session',0x137b8,(P,P,P,P,ACQUIRE,P))]:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Setup));fn.restype=U
      for i in range(512):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        def text(a,s):data=s.encode('utf-16le')+b'\0\0';state[a-A:a-A+len(data)]=data
        q(R+0x30,E);q(R+0x40,SESSION);q(SESSION+0x10,SESSION_HANDLE);q(R+0x48,0x12340000);q(R+0x50,HTTP);q(URL+0x18,HOST);q(URL+0x48,PATH);q(ACCEPT,METHOD);text(HOST,'example.test:123');text(PATH,'/path');text(METHOD,'POST');u(URL+0x20,12);u(URL+0x14,rng.randrange(4));u(E+0x180,rng.choice([0,1,2,3,4,0xffffffff]));q(E+0xf0,rng.choice([0,1,100,2**64-1]));u(R+0xd0,rng.randrange(4));state[R-A+0xd6]=rng.randrange(2)
        u(OPT+0x108,rng.randrange(7));state[OPT-A+0xfb]=rng.randrange(2);u(OPT+0x30,rng.getrandbits(32));u(OPT+0x68,rng.randrange(4));u(OPT+0x28,rng.getrandbits(32));u(OPT+0x2c,rng.getrandbits(32));q(OPT+0xa8,rng.choice([0,0x12340000]));q(OPT+0x78,rng.choice([0,1,2**64-1]));q(OPT+0x80,rng.choice([0,1,2**64-1]));u(OPT+8,rng.randrange(4))
        q(R+0x98,META);u(META,rng.getrandbits(32));u(E+0x80,rng.randrange(4));q(E+0xc8,METHOD if i%2 else 0);q(E+0xe8,ACCEPT if i%3 else 0);struct.pack_into('<H',state,E-A+0x90,80 if i%3 else 0);struct.pack_into('<H',state,URL-A+0x24,443);acq_status=rng.choice([0,1,0x80004005,0xffffffff]);port=rng.randrange(65536);scheme=rng.randrange(4);conn_ok=i%8!=0;opens=[i%3!=0,i%5!=0];oi=ni=0;ei=en=0;si=sn=0;errvals=[rng.choice([0,87,5,12002,0xffffffff,0x80004005]) for _ in range(10)];results=[rng.randrange(2) if i%4 else 1 for _ in range(12)]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def ret(mu,v):mu.reg_write(X.UC_X86_REG_RAX,v)
        def connect(mu):
            assert reg(mu,'R9')==0;oc.append(('connect',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8')&65535,bytes(mu.mem_read(HOST+24,2))));ret(mu,CONN if conn_ok else 0)
        def wide_original(addr):
            out=b'';j=0
            while (v:=bytes(m.mu.mem_read(addr+j,2)))!=b'\0\0':out+=v;j+=2
            return out
        def op(mu):
            nonlocal oi
            assert reg(mu,'R9')==0
            accepts,ref,flags=struct.unpack('<QQQ',mu.mem_read(reg(mu,'RSP')+0x28,24));assert ref==0
            oc.append(('open',reg(mu,'RCX'),wide_original(reg(mu,'RDX')),reg(mu,'R8'),accepts,flags&0xffffffff));ret(mu,OPEN_HANDLE if opens[oi] else 0);oi+=1
        def option(mu):
            nonlocal si
            assert reg(mu,'R9D')==4;v=struct.unpack('<I',mu.mem_read(reg(mu,'R8'),4))[0];oc.append(('option',reg(mu,'RCX'),reg(mu,'EDX'),v));ret(mu,results[si]);si+=1
        def timeouts(mu):
            nonlocal si
            oc.append(('timeouts',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D'),reg(mu,'R9D'),struct.unpack('<I',mu.mem_read(reg(mu,'RSP')+0x28,4))[0]));ret(mu,results[si]);si+=1
        def error(mu):
            nonlocal ei
            oc.append(('error',));ret(mu,errvals[ei]);ei+=1
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base,reg(mu,'R8D')));return 0
        def retry(mu):assert reg(mu,'RDX')==img.base+0x21520;oc.append(('retry',reg(mu,'RCX')));return 0
        imports={'WinHttpConnect':connect,'WinHttpOpenRequest':op,'WinHttpSetOption':option,'WinHttpSetTimeouts':timeouts,'GetLastError':error}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        def acquire(mu):
            out=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];assert out==R+0x40
            oc.append(('acquire',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D'),reg(mu,'R9D')))
            mu.mem_write(out,struct.pack('<Q',SESSION));return acq_status
        def setup(w):m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,ENGINE)
        args=[R,E,METHOD,URL,ACCEPT if i%2 else 0,port] if name=='http_open' else [R,E,URL] if name=='http_open_with_session' else [R,OPT,scheme]
        result,_,_,_=m.call(img.base+rva,args,'uint',setup=setup,fakes={img.base+0xa224:diag,img.base+0xa32c:retry,img.base+0x15f40:acquire})
        assert isinstance(result,int),(name,i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        @CONNECT
        def cn(h,p,pt,c):nc.append(('connect',norm(h),norm(p),pt,C.string_at(host(HOST)+24,2)));return CONN if conn_ok else None
        @OPEN
        def on(h,method,path,accept,flags,c):
            nonlocal ni
            out=b'';j=0
            while (v:=C.string_at(method+j,2))!=b'\0\0':out+=v;j+=2
            nc.append(('open',norm(h),out,norm(path),norm(accept),flags));v=OPEN_HANDLE if opens[ni] else None;ni+=1;return v
        @OPTION
        def so(h,k,v,c):
            nonlocal sn
            nc.append(('option',norm(h),k,v));v=results[sn];sn+=1;return v
        @TIMEOUT
        def st(h,a,b,d,e,c):
            nonlocal sn
            nc.append(('timeouts',norm(h),a,b,d,e));v=results[sn];sn+=1;return v
        @ERROR
        def er(c):
            nonlocal en
            nc.append(('error',));v=errvals[en];en+=1;return v
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k,v))
        @RETRY
        def re(p,c):nc.append(('retry',norm(p)))
        services=Setup(cn,on,so,st,er,di,re,None)
        @ACQUIRE
        def ac(p,e,meta,proto,out,c):nc.append(('acquire',norm(p),norm(e),meta,proto));out[0]=host(SESSION);return acq_status
        native_args=[host(R),host(E),host(URL),host(ENGINE),ac,None] if name=='http_open_with_session' else [host(v) if t==P else v for t,v in zip(types,args)]
        got=fn(*native_args,C.byref(services));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(result,oc),(name,i,got,result,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 512',flush=True)
    print('XCurl HTTP setup PASS:',total,'cases')
if __name__=='__main__':main()
