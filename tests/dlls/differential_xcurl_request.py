#!/usr/bin/env python3
"""Original request transitions and WinHTTP call boundaries, disabled ETW."""
import ctypes as C
import hashlib, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/differential'))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from unicorn import x86_const as X
P=C.c_void_p;U=C.c_uint32;Q=C.c_uint64;B=C.c_uint8
LOCK=C.CFUNCTYPE(None,P,P);ALLOC=C.CFUNCTYPE(P,C.c_size_t,U,P);FREE=C.CFUNCTYPE(None,P,U,P);ADJUST=C.CFUNCTYPE(None,U,P)
EVENT=C.CFUNCTYPE(P,P);ERROR=C.CFUNCTYPE(U,P);QUEUE=C.CFUNCTYPE(U,C.POINTER(P),P);DIAG=C.CFUNCTYPE(None,P,U,U,P)
DISPATCH=C.CFUNCTYPE(B,P,U,U,P);READ=C.CFUNCTYPE(U,P,P,U,P);RECEIVE=C.CFUNCTYPE(U,P,P);SEND=C.CFUNCTYPE(U,P,P,U,U,P,P)
SUBMIT=C.CFUNCTYPE(U,P,P,P)
CONTEXT=C.CFUNCTYPE(U,P,P,P);CALLBACK=C.CFUNCTYPE(Q,P,U,P);DEBUG=C.CFUNCTYPE(None,P,U,P,Q,P)
class Services(C.Structure):
    _fields_=[('enter',LOCK),('leave',LOCK),('allocate',ALLOC),('release',FREE),('event',LOCK),('close',LOCK),('connection',LOCK),('adjust',ADJUST),('create_event',EVENT),('error',ERROR),('queue',QUEUE),('diag',DIAG),('dispatch',DISPATCH),('read',READ),('receive',RECEIVE),('send',SEND),('context',CONTEXT),('callback',CALLBACK),('debug',DEBUG),('cancel_task',LOCK),('submit',SUBMIT),('ctx',P)]
A=0x34000000;E=A+0x1000;M=A+0x2000;MSG=A+0x3000;E2=A+0x4000;R=E+0x230;BUF=A+0x5000;SIZE=0x6000
HTTP=0x12345000;EVENT_HANDLE=0x12346000;QUEUE_HANDLE=0x12347000

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()))
    cases=[('easy_start_engine',0xb55c,(P,),U),('easy_progress',0xa8f0,(P,),None),('easy_cancel',0xca60,(P,U,U),None),('easy_done',0xb6b4,(P,),B),('request_reset',0x10ac4,(P,),None),('easy_complete',0xa6b0,(P,),None),('engine_decrement',0x16de4,(P,U),None),('http_read',0x12124,(P,P,U),U),('http_receive',0x127a8,(P,),U),('http_send',0x12d4c,(P,P,Q,Q),U),('http_register',0x13038,(P,),U),('request_close',0x13c30,(P,),None),('request_stop',0x1395c,(P,),None),('request_finish',0x13b14,(P,),None),('easy_notify',0xb90c,(P,),None),('easy_schedule_completion',0xba8c,(P,),None)]
    rng=random.Random(0xa6b0);total=0
    for name,rva,argspec,restype in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*argspec,C.POINTER(Services));fn.restype=restype
      for i in range(160):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set();o_steps=n_steps=0
        def q(addr,value):struct.pack_into('<Q',state,addr-A,value);offsets.add(addr-A)
        def u(addr,value):struct.pack_into('<I',state,addr-A,value&0xffffffff)
        def circle(head,nodes):
            chain=[head,*nodes]
            for k,p in enumerate(chain):q(p,chain[(k+1)%len(chain)]);q(p+8,chain[k-1])
        for h in (M+0x50,M+0x60,M+0x78,E,E2,MSG):circle(h,[])
        if name in ('easy_complete','easy_schedule_completion','request_finish'):
            circle(M+0x50,[E,E2] if i%3==0 else [E2,E] if i%3==1 else [E2]);circle(M+0x60,[])
        q(E+0x310,0 if name=='easy_notify' and i%2 else M);q(E+0x40,0);q(MSG+0x18,0);q(M+0xb0,EVENT_HANDLE);q(M+0xa0,EVENT_HANDLE+0x100)
        q(E+0x350,QUEUE_HANDLE if i%2 else 0);q(E+0x360,EVENT_HANDLE if i%2 else 0)
        q(R+0x30,E);q(R+0x50,HTTP);q(R+0x48,HTTP if i%2 else 0);q(R+0x40,0)
        for k,o in enumerate((0x340,0x348)):
            q(E+o,0x10000000+k*0x100 if i&(1<<k) else 0)
        for k,o in enumerate((0x90,0x98,0xb0,0xa8,0x38,0xc8)):
            q(R+o,0x11000000+k*0x100 if i&(1<<k) else 0)
        q(R+0xa0,rng.getrandbits(64));q(E+0x200,rng.getrandbits(64));u(R+0xd0,rng.getrandbits(32));u(R+0xd4,rng.getrandbits(32));u(R+0xdc,rng.getrandbits(32));state[R-A+0xb8]=i%256
        u(E+0x220,rng.getrandbits(32));u(E+0x228,[0,1,0xffffffff,0x80004004][i%4]);u(R+0x28,i%8);state[E-A+0x176]=i%3
        u(M+0x88,rng.getrandbits(32));u(M+0x3c,[0,1,2,0xffffffff][i%4]);u(A+0x40,rng.getrandbits(32))
        status=[0,1,0x80004005,0xffffffff,0x8007000e][i%5];err=[0,5,12002,0x12345678,0x80004005,0xffffffff][i%6];success=i%3!=0;callback_ok=i%4!=0;event_ok=i%3!=1;steps=i%5
        size=rng.getrandbits(64);length=rng.getrandbits(64);body=BUF if i%2 else 0;code=rng.getrandbits(32)
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def ret(mu,v):mu.reg_write(X.UC_X86_REG_RAX,v)
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def allocate(mu):oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return MSG
        def release(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def ev(mu):assert all(reg(mu,k)==0 for k in ('RCX','RDX','R8','R9'));oc.append(('create_event',));ret(mu,EVENT_HANDLE if event_ok else 0)
        def geterr(mu):oc.append(('error',));ret(mu,err)
        def create_queue(mu):assert reg(mu,'RCX')==reg(mu,'RDX')==0 and reg(mu,'R8')==E+0x350;oc.append(('queue',));mu.mem_write(E+0x350,struct.pack('<Q',QUEUE_HANDLE));return status
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base,reg(mu,'R8D')));return 0
        def dispatch(mu):
            nonlocal o_steps;o_steps+=1;oc.append(('dispatch',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D')));return int(o_steps<=steps)
        def read(mu):assert reg(mu,'R9')==0;oc.append(('read',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));ret(mu,int(success))
        def receive(mu):assert reg(mu,'RDX')==0;oc.append(('receive',reg(mu,'RCX')));ret(mu,int(success))
        def send(mu):
            assert reg(mu,'RDX')==reg(mu,'R8')==0
            ss,tt,ctx=struct.unpack('<QQQ',mu.mem_read(reg(mu,'RSP')+0x28,24));oc.append(('send',reg(mu,'RCX'),reg(mu,'R9'),ss&0xffffffff,tt&0xffffffff,ctx));ret(mu,int(success))
        def context(mu):
            assert reg(mu,'EDX')==45 and reg(mu,'R9D')==8
            oc.append(('context',reg(mu,'RCX'),struct.unpack('<Q',mu.mem_read(reg(mu,'R8'),8))[0]));ret(mu,int(success))
        def callback(mu):assert reg(mu,'RDX')==img.base+0x14a00 and reg(mu,'R9')==0;oc.append(('callback',reg(mu,'RCX'),reg(mu,'R8D')));ret(mu,0 if callback_ok else 0xffffffffffffffff)
        def debug(mu):oc.append(('debug',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8'),reg(mu,'R9')));return 0
        def cancel_task(mu):log(mu,'cancel_task');return 0
        def submit(mu):
            assert reg(mu,'EDX')==1 and reg(mu,'R9')==img.base+0xa6b0
            oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8')));return status
        def adjust(mu):oc.append(('adjust',reg(mu,'EDX')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'WinHttpCloseHandle':lambda mu:log(mu,'close'),'CreateEventW':ev,'GetLastError':geterr,'WinHttpReadData':read,'WinHttpReceiveResponse':receive,'WinHttpSendRequest':send,'WinHttpSetOption':context,'WinHttpSetStatusCallback':callback}
        for n in img.iat.values():
            k=n.split('!')[-1]
            if k in imports:m.on_import(n,imports[k])
        fakes={img.base+k:v for k,v in {0x932c:allocate,0x9640:release,0x5b10:create_queue,0xa224:diag,0x5b98:dispatch,0xa128:debug,0x16de4:adjust,0x5424:cancel_task,0x5c20:submit}.items() if not(name=='engine_decrement' and k==0x16de4)}
        def setup(w):m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,A)
        args={'easy_start_engine':[E],'easy_progress':[E],'easy_cancel':[E,status,code],'easy_done':[E],'request_reset':[R],'easy_complete':[E,0],'engine_decrement':[A,code],'http_read':[R,BUF,code],'http_receive':[R],'http_send':[R,body,size,length],'http_register':[R],'request_close':[R],'request_stop':[R],'request_finish':[R],'easy_notify':[E],'easy_schedule_completion':[E]}[name]
        # The explicit queue/HTTP callbacks are stubbed only at their boundaries; all request mutations execute original code.
        result,_,_,_=m.call(img.base+rva,args,'void' if restype is None else 'uchar' if restype==B else 'uint',setup=setup,fakes=fakes)
        assert result is None or isinstance(result,int),(name,i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return host(MSG)
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @ADJUST
        def ad(n,c):nc.append(('adjust',n))
        @EVENT
        def ce(c):nc.append(('create_event',));return EVENT_HANDLE if event_ok else None
        @ERROR
        def er(c):nc.append(('error',));return err
        @QUEUE
        def qu(p,c):nc.append(('queue',));p[0]=QUEUE_HANDLE;return status
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k,v))
        @DISPATCH
        def ds(p,k,v,c):
            nonlocal n_steps;n_steps+=1;nc.append(('dispatch',norm(p),k,v));return int(n_steps<=steps)
        @READ
        def rd(p,b,z,c):nc.append(('read',norm(p),norm(b),z));return int(success)
        @RECEIVE
        def rc(p,c):nc.append(('receive',norm(p)));return int(success)
        @SEND
        def sn(p,b,z,t,x,c):nc.append(('send',norm(p),norm(b),z,t,norm(x)));return int(success)
        @CONTEXT
        def ct(p,x,c):nc.append(('context',norm(p),norm(x)));return int(success)
        @CALLBACK
        def cb(p,k,c):nc.append(('callback',norm(p),k));return 0 if callback_ok else 0xffffffffffffffff
        @DEBUG
        def db(p,k,b,z,c):nc.append(('debug',norm(p),k,norm(b),z))
        @SUBMIT
        def sb(q,e,c):nc.append(('submit',norm(q),norm(e)));return status
        services=Services(lock('enter'),lock('leave'),al,fr,lock('event'),lock('close'),lock('connection'),ad,ce,er,qu,di,ds,rd,rc,sn,ct,cb,db,lock('cancel_task'),sb,None)
        native_args=[host(v) if t==P else v for t,v in zip(argspec,args)]
        got=fn(*native_args,C.byref(services));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(result,oc),(name,i,got,result,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 160',flush=True)
    print('XCurl original request engine PASS:',total,'cases')
if __name__=='__main__':main()
