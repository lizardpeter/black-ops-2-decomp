#!/usr/bin/env python3
"""Pinned XCurl multi/easy lifecycle, request predicate and stop wrapper."""
from differential_xcurl_request import *
from differential_xcurl_scheduler import Scheduler
from differential_xcurl_multi_queue import WAIT, CANCEL
from differential_xcurl_gdk_interfaces import Gdk, QUERYFN

CREATE_EVENT=C.CFUNCTYPE(P,B,B,P)
class MultiLife(C.Structure):
    _fields_=[('scheduler',C.POINTER(Scheduler)),('request',C.POINTER(Services)),
              ('create_mutex',EVENT),('create_event',CREATE_EVENT),('close',LOCK),
              ('release_mutex',LOCK),('error',ERROR),('ctx',P)]
class EasyLife(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('gdk',C.POINTER(Gdk)),('delete_cs',LOCK),('close',LOCK),('ctx',P)]
G=A+0x4000;E2=A+0x3000;E3=A+0x3400;NODE=A+0x5000;SIZE=0x8000
HANDLES=[0x12340000+i*0x100 for i in range(5)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()))
    rng=random.Random(0x184a8)
    specs=[('multi_start_engine',0x184a8,MultiLife,U),('multi_destroy_engine',0x18110,MultiLife,U),
           ('request_is_active',0x113b8,Services,B),('easy_stop',0xcefc,Services,None),('easy_destroy_engine',0xa544,EasyLife,None)]
    for name,rva,kind,rt in specs:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(P,P,C.POINTER(kind)) if kind==MultiLife else (P,C.POINTER(kind));fn.restype=rt
      for i in range(192):
        oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        def circle(head,nodes=()):
            chain=[head,*nodes]
            for k,p in enumerate(chain):q(p,chain[(k+1)%len(chain)]);q(p+8,chain[k-1])
        for p in [M,E,E2,E3,NODE]:circle(p)
        for o in (0x40,0x50,0x60,0x78):circle(M+o)
        q(G+0x28,HANDLES[4]);circle(G+0x30,[M] if name=='multi_destroy_engine' else [])
        state[G-A+0x48]=i%2
        for e in (E,E2,E3):
            q(e+0x310,0);u(e+0x358,rng.getrandbits(32));q(e+0x260,e);u(e+0x258,0);u(e+0x228,0);u(e+0x220,99)
        q(M+0x70,0)
        if name=='multi_destroy_engine':
            circle(M+0x40,[E] if i&1 else [])
            circle(M+0x60,[E2] if i&2 else [])
            circle(M+0x50,[E3] if i&4 else [])
            circle(M+0x78,[NODE] if i&8 else [])
            q(M+0x70,NODE if i&8 and i&16 else 0)
            for e,flag in [(E,1),(E2,2),(E3,4)]:q(e+0x310,M if i&flag else 0)
            for k,o in enumerate((0xb8,0xa0,0xa8,0xb0)):q(M+o,HANDLES[k])
            u(M+0xc4,i);u(M+0x3c,1 if i&4 else 0)
        if name in ('request_is_active','easy_stop','easy_destroy_engine'):
            u(E+0x258,[0,1,2,3,4,5,6,7,0xffffffff][i%9])
            q(E+0x280,HTTP)
        if name=='easy_destroy_engine':
            for k,o in enumerate((0x1b8,0x1d0,0x218,0x208,0x88,0xc0,0xc8,0xd8,0xe8)):
                q(E+o,0x12350000+k*0x100 if (i>>(k%7))&1 else 0)
            state[E-A+0x178]=i%2
            q(E+0x350,QUEUE_HANDLE if i&1 else 0);q(E+0x360,EVENT_HANDLE if i&2 else 0)
        failed_at=i%5 if name=='multi_start_engine' else 0
        error=[5,8,87,0x80004005][i%4];term_status=0x80004005 if i%7==0 else 0
        counters={'oc_create':0,'nc_create':0,'oc_dispatch':0,'nc_dispatch':0,'oc_wait':0,'nc_wait':0}
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def ocreate(mu,event):
            assert reg(mu,'RCX')==0
            if event:
                assert reg(mu,'R9')==0
                oc.append(('create_event',reg(mu,'EDX'),reg(mu,'R8D')))
            else:
                assert reg(mu,'RDX')==reg(mu,'R8')==0
                oc.append(('create_mutex',))
            k=counters['oc_create'];counters['oc_create']+=1
            mu.reg_write(X.UC_X86_REG_RAX,0 if failed_at==k+1 else HANDLES[k])
        def owait(mu):
            h,t=reg(mu,'RCX'),reg(mu,'EDX');oc.append(('wait',h,t))
            # Original destroy advances once again after a timed-out idle probe.
            result=0
            if name=='multi_destroy_engine' and h==HANDLES[3] and t==0:
                counters['oc_wait']+=1
                if i&16 and counters['oc_wait']==1:result=0x102
            mu.reg_write(X.UC_X86_REG_RAX,result)
        def oerror(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,error)
        def oevent(mu,k):log(mu,k);mu.reg_write(X.UC_X86_REG_RAX,1)
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base));return 0
        def terminate(mu):
            assert reg(mu,'RDX')==reg(mu,'R8')==reg(mu,'R9')==0
            oc.append(('terminate',reg(mu,'RCX')));return term_status
        def dispatch_result(key):
            counters[key]+=1
            return int(counters[key]<=2*(i%3) and counters[key]%2==1)
        def dispatch(mu):
            oc.append(('dispatch',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D')))
            return dispatch_result('oc_dispatch')
        def move_orig(mu):
            p=reg(mu,'RCX');oc.append(('progress',p))
            def rd(a):return struct.unpack('<Q',mu.mem_read(a,8))[0]
            def wr(a,v):mu.mem_write(a,struct.pack('<Q',v))
            nxt,prev=rd(p),rd(p+8);wr(nxt+8,prev);wr(prev,nxt)
            head=M+0x60;tail=rd(head+8);wr(p,head);wr(p+8,tail);wr(tail,p);wr(head+8,p)
            return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),
                 'DeleteCriticalSection':lambda mu:log(mu,'delete_cs'),'CreateMutexW':lambda mu:ocreate(mu,False),
                 'CreateEventW':lambda mu:ocreate(mu,True),'GetLastError':oerror,'WaitForSingleObject':owait,
                 'CloseHandle':lambda mu:oevent(mu,'close'),'ReleaseMutex':lambda mu:oevent(mu,'release_mutex'),
                 'WinHttpCloseHandle':lambda mu:oevent(mu,'http_close')}
        for n in img.iat.values():
            k=n.split('!')[-1]
            if k in imports:m.on_import(n,imports[k])
        fakes={img.base+k:v for k,v in {0x9640:free,0xa224:diag,0x5cbc:terminate,0x5b98:dispatch,0x5ab4:lambda mu:log(mu,'queue_close'),0xa8f0:move_orig,0x5424:lambda mu:log(mu,'cancel_task')}.items()}
        def setup(w):m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,G)
        arg=M if kind==MultiLife else R if name=='request_is_active' else E
        ret,_,_,_=m.call(img.base+rva,[arg],'void' if rt is None else 'uchar' if rt==B else 'uint',setup=setup,fakes=fakes)
        assert ret is None or isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytearray(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k))
        request=Services(lock('enter'),lock('leave'),ALLOC(),fr,LOCK(),lock('http_close'),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),di,DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),lock('cancel_task'),SUBMIT(),None)
        def ncreate():
            k=counters['nc_create'];counters['nc_create']+=1
            return 0 if failed_at==k+1 else HANDLES[k]
        @EVENT
        def mutex(c):nc.append(('create_mutex',));return ncreate()
        @CREATE_EVENT
        def event(a,b,c):nc.append(('create_event',a,b));return ncreate()
        @ERROR
        def err(c):nc.append(('error',));return error
        @WAIT
        def wait(p,t,c):
            h=norm(p);nc.append(('wait',h,t))
            if name=='multi_destroy_engine' and h==HANDLES[3] and t==0:
                counters['nc_wait']+=1
                if i&16 and counters['nc_wait']==1:return 0x102
            return 0
        lib.xcurl_research_easy_cancel.argtypes=(P,U,U,C.POINTER(Services));lib.xcurl_research_easy_cancel.restype=None
        lib.xcurl_research_easy_stop.argtypes=(P,C.POINTER(Services));lib.xcurl_research_easy_stop.restype=None
        @CANCEL
        def can(p,h,v,c):lib.xcurl_research_easy_cancel(p,h,v,C.byref(request))
        @LOCK
        def stop(p,c):lib.xcurl_research_easy_stop(p,C.byref(request))
        @LOCK
        def progress(p,c):
            nc.append(('progress',norm(p)))
            def rd(a):return C.c_uint64.from_address(a).value
            def wr(a,v):C.c_uint64.from_address(a).value=v
            nxt,prev=rd(p),rd(p+8);wr(nxt+8,prev);wr(prev,nxt)
            head=host(M+0x60);tail=rd(head+8);wr(p,head);wr(p+8,tail);wr(tail,p);wr(head+8,p)
        sch=Scheduler(lock('enter'),lock('leave'),LOCK(),progress,can,stop,LOCK(),LOCK(),wait,None)
        ml=MultiLife(C.pointer(sch),C.pointer(request),mutex,event,lock('close'),lock('release_mutex'),err,None)
        @C.CFUNCTYPE(U,P)
        def release_iface(p):return 1
        @C.CFUNCTYPE(None,P)
        def assert_thread(p):pass
        @C.CFUNCTYPE(U,P,P,B,P,P)
        def nterm(p,q,w,ctx,cb):
            assert not w and not ctx and not cb;nc.append(('terminate',norm(q)));return term_status
        @C.CFUNCTYPE(B,P,P,U,U)
        def ndispatch(p,q,port,t):nc.append(('dispatch',norm(q),port,t));return dispatch_result('nc_dispatch')
        @C.CFUNCTYPE(None,P,P)
        def nclose(p,q):nc.append(('queue_close',norm(q)))
        vt=(P*30)()
        for slot,cb in [(2,release_iface),(29,assert_thread),(22,nterm),(16,ndispatch),(17,nclose)]:vt[slot]=C.cast(cb,P).value
        obj=C.create_string_buffer(struct.pack('<Q',C.addressof(vt)))
        @QUERYFN
        def nq(cls,iid,out,c):out[0]=C.addressof(obj);return 0
        gdk=Gdk(nq,None);el=EasyLife(C.pointer(request),C.pointer(gdk),lock('delete_cs'),lock('close'),None)
        got=fn(host(arg),host(G),C.byref(ml)) if kind==MultiLife else fn(host(arg),C.byref(el if kind==EasyLife else request))
        raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        if name=='easy_destroy_engine':
            for a,b in [(0x1b4,0x1b8),(0x1c4,0x1c8),(0x1e4,0x1e8),(0x1f5,0x1f8),(0x22c,0x230),(0x3c,0x40)]:
                raw[E-A+a:E-A+b]=expected[E-A+a:E-A+b]
        assert (got,nc)==(ret,oc),(name,i,got,ret,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:40])
      print(name,'PASS 192',flush=True)
    print('XCurl remaining lifecycle PASS: 960 cases',flush=True)
if __name__=='__main__':main()
