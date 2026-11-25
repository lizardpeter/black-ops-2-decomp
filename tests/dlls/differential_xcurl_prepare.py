#!/usr/bin/env python3
"""Original request preparation and launch, using explicit URL/task boundaries."""
from differential_xcurl_request import *
from differential_xcurl_gdk_interfaces import Gdk, QUERYFN
URL=C.CFUNCTYPE(U,P,P,P,P);DEFAULT=C.CFUNCTYPE(U,P,U,C.POINTER(P),P);PORT=C.CFUNCTYPE(U,P,P);UDIAG=C.CFUNCTYPE(None,P,U,Q,U,P);COPY=C.CFUNCTYPE(U,P,P,P)
class Prepare(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('url',URL),('default',DEFAULT),('port',PORT),('udiag',UDIAG),('copy',COPY),('gdk',C.POINTER(Gdk)),('queue',P),('callback',P)]
G=A+0x4800;TEXT=0x12350000;PREPARED=0x12351000;COPIED=0x12352000;ENGINE_QUEUE=0x12353000
NET=A+0x5000;VT=A+0x5100;QUERY=A+0x5200;RELEASE=A+0x5210;SECURITY=A+0x5220

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0xcf90);total=0
    for name,rva,typ,restype in [('easy_reset_transfer',0xc6a0,Services,None),('easy_prepare_url',0xd174,Prepare,U),('request_begin',0x11780,Prepare,None),('easy_begin',0xcf90,Prepare,U)]:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(P,C.POINTER(typ));fn.restype=restype
      for i in range(256):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        state[E-A:E-A+0x368]=rng.randbytes(0x368)
        q(E+0x78,0x12354000 if i&1 else 0);q(E+0x88,0x12355000 if i&2 else 0);struct.pack_into('<H',state,E-A+0x90,80 if i&4 else 0)
        for k,o in enumerate((0x1b8,0x1d0,0x208,0x218)):
            q(E+o,0x12356000+0x100*k if i&(1<<k) else 0)
        q(E+0x310,M);q(E+0x350,QUEUE_HANDLE);q(E+0x360,EVENT_HANDLE);q(M+0xa0,EVENT_HANDLE+0x100);q(G+0x90,ENGINE_QUEUE)
        q(R+0x30,E);q(R+0x38,COPIED if i&8 else 0);q(R+0x50,HTTP);u(R+0x28,i%7);u(E+0x228,rng.choice([0,1,0x80004004]));u(E+0x220,99)
        offsets.update((R-A+0x58,R-A+0x60,R-A+0x68))
        for off in (0x58,0x60,0x68):q(R+off,0x12358000+off)
        statuses=[0,1,0x8007000e,0x80004005,0xffffffff];url_result=rng.choice(statuses);default_result=rng.choice([0,1,27]);copy_result=rng.choice(statuses);queue_result=rng.choice(statuses);port=rng.getrandbits(32)
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def url(mu):
            oc.append(('url',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8')))
            if not url_result&0x80000000:mu.mem_write(reg(mu,'R8'),struct.pack('<Q',PREPARED))
            return url_result
        def default(mu):
            assert reg(mu,'R9D')==1
            oc.append(('default',reg(mu,'RCX'),reg(mu,'EDX')))
            if not default_result:mu.mem_write(reg(mu,'R8'),struct.pack('<Q',TEXT))
            return default_result
        def parse(mu):oc.append(('port',reg(mu,'RCX')));mu.reg_write(X.UC_X86_REG_RAX,port)
        def copy(mu):
            oc.append(('copy',reg(mu,'RCX'),reg(mu,'R8')))
            if not copy_result&0x80000000:mu.mem_write(reg(mu,'R8'),struct.pack('<Q',COPIED))
            return copy_result
        q(NET,VT);q(VT+2*8,RELEASE);q(VT+11*8,SECURITY)
        def query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16));oc.append(('query',cls,iid));mu.mem_write(reg(mu,'R8'),struct.pack('<Q',NET));return 0
        def release(mu):oc.append(('release_iface',reg(mu,'RCX')));return 1
        def queue(mu):oc.append(('queue',reg(mu,'RDX'),reg(mu,'R8')));return queue_result
        def diag(mu):
            site=reg(mu,'RDX')-img.base
            if site==0x207e8:oc.append(('udiag',reg(mu,'RCX'),site,reg(mu,'R8'),reg(mu,'R9D')))
            elif site in (0x20810,0x20830,0x20848):oc.append(('udiag',reg(mu,'RCX'),site,reg(mu,'R8D'),0))
            else:oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x20de8 else reg(mu,'R8D')))
            return 0
        def submit(mu):oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'WinHttpCloseHandle':lambda mu:log(mu,'close')}
        for addr,n in img.iat.items():
            if addr==img.base+0x1f7e0:m.on_import(n,parse)
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        fakes={img.base+k:v for k,v in {0x9640:free,0x10ea4:url,0x1d170:default,0x94ac:copy,0xa224:diag,0xa32c:diag,0x5424:lambda mu:log(mu,'cancel_task'),0x5c20:submit}.items()}
        fakes.update({QUERY:query,RELEASE:release,SECURITY:queue})
        def setup(w):
            m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,G);m.mu.mem_write(img.base+0x289d0,bytes([1,0]));m.mu.mem_write(img.base+0x289e0,struct.pack('<Q',QUERY))
        arg=R if name=='request_begin' else E
        ret,_,_,_=m.call(img.base+rva,[arg],'void' if restype is None else 'uint',setup=setup,fakes=fakes)
        assert ret is None or isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytearray(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k,v))
        @SUBMIT
        def sb(q,e,c):nc.append(('submit',norm(q),norm(e)));return 0
        services=Services(lock('enter'),lock('leave'),ALLOC(),fr,lock('event'),lock('close'),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),di,DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),lock('cancel_task'),sb,None)
        @URL
        def nu(p,e,o,c):
            nc.append(('url',norm(p),norm(e),norm(o)))
            if not url_result&0x80000000:C.cast(o,C.POINTER(P))[0]=PREPARED
            return url_result
        @DEFAULT
        def nd(p,k,o,c):
            nc.append(('default',norm(p),k))
            if not default_result:o[0]=TEXT
            return default_result
        @PORT
        def np(p,c):nc.append(('port',norm(p)));return port
        @UDIAG
        def ud(p,k,a,v,c):nc.append(('udiag',norm(p),k,norm(a),v))
        @COPY
        def cp(p,o,c):
            nc.append(('copy',norm(p),norm(o)))
            if not copy_result&0x80000000:C.cast(o,C.POINTER(P))[0]=COPIED
            return copy_result
        @C.CFUNCTYPE(U,P)
        def rel(p):nc.append(('release_iface',NET));return 1
        @C.CFUNCTYPE(U,P,P,P)
        def qs(self,p,o):nc.append(('queue',norm(p),norm(o)));return queue_result
        vt=(P*16)();vt[2]=C.cast(rel,P).value;vt[11]=C.cast(qs,P).value;obj=C.create_string_buffer(struct.pack('<Q',C.addressof(vt)))
        @QUERYFN
        def nq(cls,iid,out,c):nc.append(('query',C.string_at(cls,16),C.string_at(iid,16)));out[0]=C.addressof(obj);return 0
        gdk=Gdk(nq,None)
        prep=Prepare(C.pointer(services),nu,nd,np,ud,cp,C.pointer(gdk),ENGINE_QUEUE,img.base+0x11f10)
        got=fn(host(arg),C.byref(services if typ==Services else prep));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(ret,oc),(name,i,got,ret,nc,oc)
        # Original stack aggregate's unwritten padding is excluded.
        if name in ('easy_reset_transfer','easy_begin'):
            for a,b in [(0x1b4,0x1b8),(0x1c4,0x1c8),(0x1e4,0x1e8),(0x1f5,0x1f8),(0x22c,0x230),(0x3c,0x40)]:
                raw[E-A+a:E-A+b]=expected[E-A+a:E-A+b]
        assert raw==expected,(name,i,[(hex(j-E+A),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:40])
      print(name,'PASS 256',flush=True)
    print('XCurl preparation PASS:',total,'cases')
if __name__=='__main__':main()
