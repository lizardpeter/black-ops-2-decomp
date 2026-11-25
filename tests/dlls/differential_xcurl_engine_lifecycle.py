#!/usr/bin/env python3
"""Original engine startup/shutdown, connectivity and suspend/resume, including actual GDK and list helpers."""
from differential_xcurl_request import *
from differential_xcurl_session import Session, OPEN, OPTION
from differential_xcurl_scheduler import Scheduler
from differential_xcurl_multi_queue import WAIT, CANCEL
from differential_xcurl_gdk_interfaces import Gdk, QUERYFN
CREATE=C.CFUNCTYPE(U,C.POINTER(P),P);APP=C.CFUNCTYPE(U,P,P,C.POINTER(P),P);FREQ=C.CFUNCTYPE(U,C.POINTER(Q),P)
class Global(C.Structure):
    _fields_=[('engine',P),('references',U)]
class Engine(C.Structure):
    _fields_=[('gdk',C.POINTER(Gdk)),('sessions',C.POINTER(Session)),('scheduler',C.POINTER(Scheduler)),
      ('init_cs',LOCK),('delete_cs',LOCK),('enter',LOCK),('leave',LOCK),('event',LOCK),('close',LOCK),('release_mutex',LOCK),
      ('wait',WAIT),('create_mutex',EVENT),('error',ERROR),('telemetry_create',CREATE),('telemetry_destroy',LOCK),
      ('register_app',APP),('unregister_app',LOCK),('connectivity_callback',P),('app_callback',P),('allocate',ALLOC),('release',FREE),('runtime_init',ERROR),('frequency',FREQ),('ctx',P)]
G=A+0x100;MULTIS=[A+0x1000,A+0x1800,A+0x2000];NEW=A+0x2800;EASIES=[A+0x3000,A+0x3800,A+0x4000];ENTRY=A+0x5000;SESSION=A+0x5200;AGENT=A+0x5400;REQUEST=A+0x5600
ASYNC=A+0x6000;NET=A+0x6100;AVT=A+0x6200;NVT=A+0x6300;QUERY=A+0x6500;RELEASE=A+0x6510;ASSERT=A+0x6520;CREATEQ=A+0x6530;TERM=A+0x6540;CLOSEQ=A+0x6550;HINT=A+0x6560;REG=A+0x6570;UNREG=A+0x6580;VERIFY=A+0x6590
SIZE=0x8000;MUTEX=0x12340000;QUEUE_H=0x12341000;APP_H=0x12342000;TELEMETRY=0x12343000;TOKEN=0x12344000;FREQUENCY=10_000_000

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x164d4);total=0
    thread_guid=bytes(m.mu.mem_read(img.base+0x20120,16));network_class=bytes(m.mu.mem_read(img.base+0x20130,16));network_iid=bytes(m.mu.mem_read(img.base+0x20140,16));connect_cb=img.base+0x16960;app_cb=img.base+0x16b60;vtable=img.base+0x1f058
    cases=[('engine_construct',0x164d4,(P,),P,'uint64'),('engine_initialize',0x16c04,(P,),U,'uint'),('engine_destroy',0x165d0,(P,),None,'void'),('engine_connectivity',0x166ec,(P,),None,'void'),('engine_set_limit',0x17044,(P,U),None,'void'),('engine_suspend',0x170f4,(P,),None,'void'),('engine_resume',0x16ec4,(P,),None,'void'),('engine_app_state',0x16b60,(B,P),None,'void'),('engine_create',0x169fc,(C.POINTER(Global),),U,'uint'),('engine_bootstrap',0x9ba0,(C.POINTER(Global),U,C.POINTER(Q)),U,'uint'),('verify_server_certificate',0x13d50,(P,),U,'uint')]
    for name,rva,types,restype,mret in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Gdk if name=='verify_server_certificate' else Engine));fn.restype=restype
      for i in range(256):
        total+=1;state=bytearray(SIZE);offsets=set();oc=[];nc=[]
        def q(a,v):struct.pack_into('<Q',state,a-A,v&0xffffffffffffffff);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        def circle(head,nodes):
            chain=[head,*nodes]
            for k,p in enumerate(chain):q(p,chain[(k+1)%len(chain)]);q(p+8,chain[k-1])
        state[G-A:G-A+0xa8]=rng.randbytes(0xa8) if name in ('engine_construct','engine_create','engine_bootstrap') else bytes(0xa8)
        q(G+0x28,MUTEX if i%2 else 0);nodes=MULTIS[:i%4];circle(G+0x30,nodes);u(G+0x40,rng.getrandbits(32));u(G+0x44,rng.getrandbits(32));state[G-A+0x48]=i%2;state[G-A+0x49]=(i//2)%2
        for j,n in enumerate(MULTIS+[NEW]):
            if n not in nodes:circle(n,[])
            circle(n+0x50,[EASIES[j]] if j<3 and (i+j)%3 else []);q(n+0xa0,0x12350000+j*0x100);q(n+0xb0,0x12360000+j*0x100);state[n-A+0xc0]=i%2
        for j,e in enumerate(EASIES):
            if (i+j)%3==0:circle(e,[])
        has_cache=i%3!=0 and name not in ('engine_construct','engine_create','engine_bootstrap','engine_initialize');circle(G+0x78,[ENTRY] if has_cache else []);circle(ENTRY,[] if not has_cache else [ ] ) if not has_cache else None
        q(ENTRY+0x18,AGENT);q(ENTRY+0x28,SESSION);q(SESSION,vtable);u(SESSION+8,1+i%3);q(SESSION+0x10,HTTP if i%2 else 0)
        q(G+0x88,TOKEN if i%2 else 0);q(G+0x90,QUEUE_H if i%2 else 0);q(G+0x98,TELEMETRY if i%3 else 0);q(G+0xa0,APP_H if i%4 else 0)
        q(REQUEST+0x50,HTTP);q(REQUEST+0x98,TELEMETRY if i%2 else 0)
        q(ASYNC,AVT);q(NET,NVT)
        for slot,address in [(2,RELEASE),(29,ASSERT),(12,CREATEQ),(22,TERM),(17,CLOSEQ)]:q(AVT+slot*8,address)
        for slot,address in [(2,RELEASE),(15,HINT),(16,REG),(17,UNREG),(14,VERIFY)]:q(NVT+slot*8,address)
        stage=i%7;status=0x80004005;tele_hr=status if stage==0 else 0;queue_hr=status if stage==1 else 0;mutex_ok=stage!=2;reg_hr=status if stage==3 else 0;app_hr=status if stage==4 else i%2
        alloc_ok=i%11!=0 or name=='engine_bootstrap';ready=i%3!=0;hint_hr=status if i%7==0 else 0;verify_hr=rng.choice([0,1,status]);error=rng.choice([5,87,0x80004005]);limit=rng.getrandbits(32);quiesced=i%2
        runtime_hr=status if i%9==0 else 0;global_initial=G if name=='engine_bootstrap' and i%5==0 else 0
        race=bool(nodes) and i%4==3 and (name=='engine_suspend' or name=='engine_app_state' and quiesced)
        added_o=added_n=False
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def writeq(mu,p,v):mu.mem_write(p,struct.pack('<Q',v))
        def create_cs(mu):assert reg(mu,'EDX')==0 and reg(mu,'R8D')==0;oc.append(('init_cs',reg(mu,'RCX')));return 1
        def mutex(mu):assert reg(mu,'RCX')==reg(mu,'RDX')==reg(mu,'R8')==0;oc.append(('mutex',));return MUTEX if mutex_ok else 0
        def wait(mu):
            nonlocal added_o
            handle=reg(mu,'RCX');oc.append(('wait',handle,reg(mu,'EDX')))
            if race and not added_o and handle!=MUTEX:
                added_o=True;head=G+0x30;tail=struct.unpack('<Q',mu.mem_read(head+8,8))[0];writeq(mu,tail,NEW);writeq(mu,NEW+8,tail);writeq(mu,NEW,head);writeq(mu,head+8,NEW)
            return 0
        def telemetry(mu):p=reg(mu,'RCX');oc.append(('telemetry_create',p));writeq(mu,p,0 if tele_hr else TELEMETRY);return tele_hr
        def app(mu):p=reg(mu,'R8');oc.append(('app_register',reg(mu,'RCX'),reg(mu,'RDX'),p));writeq(mu,p,0 if app_hr&0x80000000 else APP_H);return app_hr
        def query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16));oc.append(('query',cls,iid));writeq(mu,reg(mu,'R8'),ASYNC if cls==thread_guid else NET);return 0
        def release_iface(mu):oc.append(('release_iface',reg(mu,'RCX')));return 1
        def assert_thread(mu):oc.append(('assert_thread',));return 0
        def create_queue(mu):p=reg(mu,'R9');oc.append(('queue_create',reg(mu,'EDX'),reg(mu,'R8D'),p));writeq(mu,p,0 if queue_hr else QUEUE_H);return queue_hr
        def terminate(mu):cb=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];oc.append(('terminate',reg(mu,'RDX'),reg(mu,'R8')&0xff,reg(mu,'R9'),cb));return 0
        def close_queue(mu):oc.append(('close_queue',reg(mu,'RDX')));return 0
        def hint(mu):oc.append(('hint',));mu.mem_write(reg(mu,'RDX'),bytes(12)+bytes([ready])+bytes(3));return hint_hr
        def register(mu):p=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];oc.append(('register',reg(mu,'RDX'),reg(mu,'R8'),reg(mu,'R9'),p));writeq(mu,p,0 if reg_hr else TOKEN);return reg_hr
        def unregister(mu):oc.append(('unregister',reg(mu,'RDX'),reg(mu,'R8')&0xff));return 1
        def verify(mu):oc.append(('verify',reg(mu,'RDX'),reg(mu,'R8')));return verify_hr
        def alloc(mu):oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return G if alloc_ok else 0
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def cancel(mu):oc.append(('cancel',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D')));return 0
        def freq(mu):oc.append(('frequency',));writeq(mu,reg(mu,'RCX'),FREQUENCY);return 1
        imports={'InitializeCriticalSectionEx':create_cs,'DeleteCriticalSection':lambda mu:log(mu,'delete_cs'),'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'CloseHandle':lambda mu:log(mu,'close_handle'),'ReleaseMutex':lambda mu:log(mu,'release_mutex'),'WaitForSingleObject':wait,'CreateMutexW':mutex,'GetLastError':lambda mu:error,'RegisterAppStateChangeNotification':app,'UnregisterAppStateChangeNotification':lambda mu:log(mu,'app_unregister'),'WinHttpCloseHandle':lambda mu:log(mu,'close_http'),'QueryPerformanceFrequency':freq}
        def imported(fn):
            def call(mu):
                result=fn(mu)
                if result is not None:mu.reg_write(X.UC_X86_REG_RAX,result&0xffffffffffffffff)
            return call
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imported(imports[n.split('!')[-1]]))
        def setup(w):
            m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,global_initial);w.u32(img.base+0x282bc,limit);w.u64(img.base+0x28b38,0)
            m.mu.mem_write(img.base+0x289d0,bytes([1,0]));m.mu.mem_write(img.base+0x289e0,struct.pack('<Q',QUERY))
        fakes={QUERY:query,RELEASE:release_iface,ASSERT:assert_thread,CREATEQ:create_queue,TERM:terminate,CLOSEQ:close_queue,HINT:hint,REG:register,UNREG:unregister,VERIFY:verify,
               img.base+0x7df4:telemetry,img.base+0x80b8:lambda mu:log(mu,'telemetry_destroy'),img.base+0x932c:alloc,img.base+0x9640:free,img.base+0xca60:cancel,img.base+0xcefc:lambda mu:log(mu,'after_cancel'),img.base+0x5560:lambda mu:(oc.append(('runtime_init',)),runtime_hr)[1]}
        args=[] if name in ('engine_create','engine_bootstrap') else [G,limit] if name=='engine_set_limit' else [quiesced,G] if name=='engine_app_state' else [REQUEST] if name=='verify_server_certificate' else [G]
        ret,_,writes,_=m.call(img.base+rva,args,mret,setup=setup,fakes=fakes)
        assert not isinstance(ret,str),(name,i,ret,oc,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));glob_bytes=struct.pack('<Q',global_initial);expected_global=struct.unpack('<Q',bytes(writes.get(img.base+0x28c10+k,bytes([v]))[0] for k,v in enumerate(glob_bytes)))[0]
        native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def host(v):return base+v-A if A<=v<A+SIZE else v
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def action(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @WAIT
        def nwait(h,t,c):
            nonlocal added_n
            nc.append(('wait',norm(h),t))
            if race and not added_n and h!=MUTEX:
                added_n=True;head=host(G+0x30);tail=C.cast(head+8,C.POINTER(P))[0];C.cast(tail,C.POINTER(P))[0]=host(NEW);C.cast(host(NEW)+8,C.POINTER(P))[0]=tail;C.cast(host(NEW),C.POINTER(P))[0]=head;C.cast(head+8,C.POINTER(P))[0]=host(NEW)
            return 0
        @EVENT
        def nmutex(c):nc.append(('mutex',));return MUTEX if mutex_ok else None
        @ERROR
        def nerror(c):return error
        @CREATE
        def ntel(out,c):nc.append(('telemetry_create',norm(C.cast(out,P).value)));out[0]=None if tele_hr else TELEMETRY;return tele_hr
        @APP
        def napp(cb,g,out,c):nc.append(('app_register',norm(cb),norm(g),norm(C.cast(out,P).value)));out[0]=None if app_hr&0x80000000 else APP_H;return app_hr
        @ALLOC
        def nal(n,tag,c):nc.append(('alloc',n,tag));return host(G) if alloc_ok else None
        @FREE
        def nfree(p,tag,c):nc.append(('free',norm(p),tag))
        @CANCEL
        def ncancel(p,h,k,c):nc.append(('cancel',norm(p),h,k))
        @ERROR
        def ninit(c):nc.append(('runtime_init',));return runtime_hr
        @FREQ
        def nfreq(p,c):nc.append(('frequency',));p[0]=FREQUENCY;return 1
        @C.CFUNCTYPE(U,P)
        def nrel(p):nc.append(('release_iface',ASYNC if p==C.addressof(ao) else NET));return 1
        @C.CFUNCTYPE(None,P)
        def nassert(p):nc.append(('assert_thread',))
        @C.CFUNCTYPE(U,P,U,U,C.POINTER(P))
        def ncreate(p,w,c,out):nc.append(('queue_create',w,c,norm(C.cast(out,P).value)));out[0]=None if queue_hr else QUEUE_H;return queue_hr
        @C.CFUNCTYPE(U,P,P,B,P,P)
        def nterm(p,q,wait,ctx,cb):nc.append(('terminate',norm(q),wait,norm(ctx),norm(cb)));return 0
        @C.CFUNCTYPE(None,P,P)
        def nclose(p,q):nc.append(('close_queue',norm(q)))
        @C.CFUNCTYPE(U,P,P)
        def nhint(p,out):nc.append(('hint',));C.memmove(out,bytes(12)+bytes([ready])+bytes(3),16);return hint_hr
        @C.CFUNCTYPE(U,P,P,P,P,P)
        def nreg(p,q,g,cb,out):nc.append(('register',norm(q),norm(g),norm(cb),norm(out)));C.cast(out,C.POINTER(Q))[0]=0 if reg_hr else TOKEN;return reg_hr
        @C.CFUNCTYPE(B,P,Q,B)
        def nunreg(p,t,wait):nc.append(('unregister',t,wait));return 1
        @C.CFUNCTYPE(U,P,P,P)
        def nverify(p,h,meta):nc.append(('verify',norm(h),norm(meta)));return verify_hr
        av=(P*32)();nv=(P*32)();ao=C.create_string_buffer(8);no=C.create_string_buffer(8)
        for vt,items in [(av,[(2,nrel),(29,nassert),(12,ncreate),(22,nterm),(17,nclose)]),(nv,[(2,nrel),(15,nhint),(16,nreg),(17,nunreg),(14,nverify)])]:
            for slot,cb in items:vt[slot]=C.cast(cb,P).value
        C.memmove(ao,struct.pack('<Q',C.addressof(av)),8);C.memmove(no,struct.pack('<Q',C.addressof(nv)),8)
        @QUERYFN
        def nq(cls,iid,out,c):
            a,b=C.string_at(cls,16),C.string_at(iid,16);nc.append(('query',a,b));out[0]=C.addressof(ao) if a==thread_guid else C.addressof(no);return 0
        gd=Gdk(nq,None);ss=Session(action('enter'),action('leave'),nal,nfree,OPEN(),OPTION(),nerror,action('close_http'),DIAG(),vtable,None)
        sc=Scheduler(action('enter'),action('leave'),LOCK(),LOCK(),ncancel,action('after_cancel'),LOCK(),action('event'),nwait,None)
        svc=Engine(C.pointer(gd),C.pointer(ss),C.pointer(sc),action('init_cs'),action('delete_cs'),action('enter'),action('leave'),action('event'),action('close_handle'),action('release_mutex'),nwait,nmutex,nerror,ntel,action('telemetry_destroy'),napp,action('app_unregister'),connect_cb,app_cb,nal,nfree,ninit,nfreq,None)
        glob=Global(host(global_initial) if global_initial else None,1);frequency=Q(0)
        if name=='engine_create':got=fn(C.byref(glob),C.byref(svc))
        elif name=='engine_bootstrap':got=fn(C.byref(glob),limit,C.byref(frequency),C.byref(svc))
        elif name=='engine_set_limit':got=fn(host(G),limit,C.byref(svc))
        elif name=='engine_app_state':got=fn(quiesced,host(G),C.byref(svc))
        elif name=='verify_server_certificate':got=fn(host(REQUEST),C.byref(gd))
        else:got=fn(host(G),C.byref(svc))
        if restype is P:got=norm(got)
        raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert got==ret and oc==nc,(name,i,got,ret,oc,nc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:32])
        if name in ('engine_create','engine_bootstrap'):assert norm(glob.engine)==expected_global,(name,i,norm(glob.engine),expected_global)
        if name=='engine_bootstrap':assert frequency.value==(FREQUENCY if ('frequency',) in oc else 0)
      print(name,'PASS 256',flush=True)
    print('XCurl original engine lifecycle PASS:',total,'comparisons')
if __name__=='__main__':main()
