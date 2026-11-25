#!/usr/bin/env python3
"""Original 13E14 notification dispatch, disabled ETW; unported helpers are explicit boundaries."""
from differential_xcurl_request import *
from differential_xcurl_gdk_interfaces import Gdk, QUERYFN
THREAD=A+0x5100;NET=A+0x5200;TVT=A+0x5300;NVT=A+0x5400;QUERY=A+0x5500;RELEASE=A+0x5510;ASSERT=A+0x5520;VERIFY=A+0x5530
ONE=C.CFUNCTYPE(U,P,P);TWO=C.CFUNCTYPE(U,P,P,P);HEAD=C.CFUNCTYPE(U,P,P,C.POINTER(U),P);DATA=C.CFUNCTYPE(U,P,P,U,P);SD=C.CFUNCTYPE(None,P,U,Q,Q,U,P)
class Status(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('gdk',C.POINTER(Gdk)),('redirect',TWO),('upload',ONE),('headers',HEAD),('publish_headers',ONE),('data',DATA),('schedule',TWO),('query',ONE),('diag',SD)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));fn=lib.xcurl_research_http_status;fn.argtypes=(P,U,P,U,C.POINTER(Status));fn.restype=None
    rng=random.Random(0x13e14);total=0;thread_guid=bytes(m.mu.mem_read(img.base+0x20120,16))
    for status in (1,2,4,8,16,0x400,0x800,0x4000,0x10000,0x20000,0x40000,0x80000,0x100000,0x200000,0x400000,0xffffffff):
      for i in range(240):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        q(R+0x30,E);q(R+0x50,HTTP);q(R+0x38,0x11110000 if i%2 else 0);q(R+0xb0,0x22220000 if i%3 else 0);q(E+0x310,M);q(E+0x350,QUEUE_HANDLE);q(E+0x360,EVENT_HANDLE);q(M+0xa0,EVENT_HANDLE+0x100)
        u(R+0x28,rng.randrange(8));u(R+0xd0,rng.randrange(3));state[R-A+0xd5]=rng.randrange(2);state[R-A+0xb8]=rng.randrange(2);state[E-A+0x174]=rng.randrange(2);state[E-A+0x177]=rng.randrange(2);u(E+0x80,rng.randrange(5));u(E+0x228,rng.choice([0,1,0x80004004]));u(E+0x220,99)
        struct.pack_into('<QQ',state,R-A+0xc0,rng.choice([0,1,20,100,2**64-1]),rng.choice([0,1,20,100,2**64-1]));api=rng.randrange(8);error=rng.choice([0,5,12002,12156,0x80004005,0xffffffff]);n=rng.choice([0,1,20,100,0xffffffff]);length=rng.choice([0,1,20,100,0xffffffff]);http=rng.choice([200,399,400,404,500]);results=[rng.choice([0,1,0x80004005,0xffffffff]) for _ in range(7)];query_ok=rng.randrange(2);receive_ok=rng.randrange(2)
        if status==0x200000:struct.pack_into('<QI',state,BUF-A,api,error)
        else:u(BUF,n)
        q(R+0x98,0x12345000 if i%3 else 0);q(THREAD,TVT);q(NET,NVT);q(TVT+2*8,RELEASE);q(TVT+29*8,ASSERT);q(NVT+2*8,RELEASE);q(NVT+14*8,VERIFY)
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def step(mu,k,j,count=1):oc.append((k,*[reg(mu,x) for x in ('RCX','RDX','R8D')[:count]]));return results[j]
        def redirect(mu):oc.append(('redirect',reg(mu,'RCX'),reg(mu,'R8')));return results[1]
        def headers(mu):oc.append(('headers',reg(mu,'RCX'),reg(mu,'RDX')));mu.mem_write(reg(mu,'R8'),struct.pack('<I',http));return results[3]
        def diag(mu):
            site=reg(mu,'RDX')-img.base;a=reg(mu,'R8') if site in (0x219a8,0x21970,0x21900) else reg(mu,'R8D');b=reg(mu,'R9') if site in (0x21970,0x21900) else 0;c=struct.unpack('<I',mu.mem_read(reg(mu,'RSP')+0x28,4))[0] if b or site in (0x21970,0x21900) else 0;oc.append(('diag',reg(mu,'RCX'),site,a,b,c));return 0
        def release(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def oscall(mu,k,v):log(mu,k);mu.reg_write(X.UC_X86_REG_RAX,v)
        def err(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,error)
        def submit(mu):oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'WinHttpCloseHandle':lambda mu:log(mu,'close'),'WinHttpReceiveResponse':lambda mu:oscall(mu,'receive',receive_ok),'WinHttpQueryDataAvailable':lambda mu:oscall(mu,'query',query_ok),'GetLastError':err}
        for name in img.iat.values():
            if name.split('!')[-1] in imports:m.on_import(name,imports[name.split('!')[-1]])
        def gdk_query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16));oc.append(('gdk_query',cls,iid));mu.mem_write(reg(mu,'R8'),struct.pack('<Q',THREAD if cls==thread_guid else NET));return 0
        def iface_release(mu):oc.append(('iface_release',reg(mu,'RCX')));return 1
        def assert_thread(mu):oc.append(('assert_thread',));return 0
        def verify(mu):oc.append(('verify',reg(mu,'RDX'),reg(mu,'R8')));return results[0]
        fakes={img.base+k:v for k,v in {0x94ac:redirect,0xbf6c:lambda mu:step(mu,'upload',2),0x12228:headers,0xbba8:lambda mu:step(mu,'publish_headers',4),0xbdb0:lambda mu:step(mu,'data',5,3),0x11fa8:lambda mu:step(mu,'schedule',6,2),0xa224:diag,0x9640:release,0x5c20:submit}.items()}
        fakes.update({QUERY:gdk_query,RELEASE:iface_release,ASSERT:assert_thread,VERIFY:verify})
        def setup(w):
            m.mu.mem_write(A,bytes(state));m.mu.mem_write(img.base+0x289d0,bytes([1,0]));m.mu.mem_write(img.base+0x289e0,struct.pack('<Q',QUERY))
        result,_,_,_=m.call(img.base+0x13e14,[R,status,BUF,length],'void',setup=setup,fakes=fakes);assert result is None,(status,i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @ERROR
        def er(c):nc.append(('error',));return error
        @RECEIVE
        def rc(p,c):nc.append(('receive',norm(p)));return receive_ok
        @SUBMIT
        def sb(q,e,c):nc.append(('submit',norm(q),norm(e)));return 0
        services=Services(lock('enter'),lock('leave'),ALLOC(),fr,lock('event'),lock('close'),LOCK(),ADJUST(),EVENT(),er,QUEUE(),DIAG(),DISPATCH(),READ(),rc,SEND(),CONTEXT(),CALLBACK(),DEBUG(),LOCK(),sb,None)
        def one(k,j):return ONE(lambda p,c:(nc.append((k,norm(p))),results[j])[1])
        def two(k,j):return TWO(lambda p,v,c:(nc.append((k,norm(p),norm(v))),results[j])[1])
        @HEAD
        def hd(p,e,out,c):nc.append(('headers',norm(p),norm(e)));out[0]=http;return results[3]
        @DATA
        def dt(p,v,l,c):nc.append(('data',norm(p),norm(v),l));return results[5]
        @ONE
        def qu(p,c):nc.append(('query',norm(p)));return query_ok
        @SD
        def di(p,k,a,b,v,c):nc.append(('diag',norm(p),k,norm(a) if k==0x219a8 else a,b,v))
        tv=(P*32)();nv=(P*32)();to=C.create_string_buffer(8);no=C.create_string_buffer(8)
        @C.CFUNCTYPE(U,P)
        def rel(p):nc.append(('iface_release',THREAD if p==C.addressof(to) else NET));return 1
        @C.CFUNCTYPE(None,P)
        def ast(p):nc.append(('assert_thread',))
        @C.CFUNCTYPE(U,P,P,P)
        def ver(p,h,meta):nc.append(('verify',norm(h),norm(meta)));return results[0]
        tv[2]=nv[2]=C.cast(rel,P).value;tv[29]=C.cast(ast,P).value;nv[14]=C.cast(ver,P).value
        C.memmove(to,struct.pack('<Q',C.addressof(tv)),8);C.memmove(no,struct.pack('<Q',C.addressof(nv)),8)
        @QUERYFN
        def nq(cls,iid,out,c):
            a,b=C.string_at(cls,16),C.string_at(iid,16);nc.append(('gdk_query',a,b));out[0]=C.addressof(to) if a==thread_guid else C.addressof(no);return 0
        gdk=Gdk(nq,None)
        ss=Status(C.pointer(services),C.pointer(gdk),two('redirect',1),one('upload',2),hd,one('publish_headers',4),dt,two('schedule',6),qu,di)
        fn(host(R),status,host(BUF),length,C.byref(ss));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert nc==oc,(hex(status),i,nc,oc)
        assert raw==expected,(hex(status),i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(hex(status),'PASS 240',flush=True)
    print('XCurl original WinHTTP status PASS:',total,'cases')
if __name__=='__main__':main()
