#!/usr/bin/env python3
"""Original double-buffer reservation, read scheduling and queued delivery."""
from differential_xcurl_request import *
BSUBMIT=C.CFUNCTYPE(U,P,P,U,P);DELIVER=C.CFUNCTYPE(U,P,P,U,P)
class Body(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('submit',BSUBMIT),('deliver',DELIVER)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0xaffc)
    cases=[('reserve_body_buffer',0xaffc,(P,P),U),('queue_read',0xbcac,(P,),U),('schedule_read',0x11fa8,(P,P),U),('deferred_read',0xb800,(P,),None),('publish_body',0xbdb0,(P,P,U),U),('body_worker',0xc4a0,(P,),None)]
    total=0;OUT=A+0x80;B0=A+0x5000;B1=A+0x5100;NEW=A+0x5200
    for name,rva,argspec,restype in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*argspec,C.POINTER(Body));fn.restype=restype
      for i in range(256):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        q(E+0x310,M if i%2 else 0);q(M+0xa0,EVENT_HANDLE+0x100);q(E+0x350,QUEUE_HANDLE);q(E+0x360,EVENT_HANDLE);q(R+0x30,E);q(R+0x50,HTTP);q(OUT,0x77777777)
        q(E+0x340,B0 if i&4 or name in ('publish_body','body_worker') else 0);q(E+0x348,B1 if i&8 or name in ('publish_body','body_worker') else 0)
        u(E+0x338,(rng.getrandbits(32)&~3)|(i%4));u(E+0x228,[0,1,0x80004005,0xffffffff][i%4]);u(E+0x220,77)
        u(R+0x28,2 if i%4 else i%7);u(R+0xd8,[0,1,2,0xffffffff][(i//4)%4]);u(R+0xdc,[0,1,0x2000,0x2001,0xffffffff][i%5]);state[R-A+0xb8]=i%2
        idx=i%2;task=E+0x318+idx*16;length=rng.getrandbits(32);q(task,E);u(task+8,idx);u(task+12,length)
        status=[0,1,0x80004005,0xffffffff][(i//8)%4];alloc_ok=i%7!=0;read_ok=i%5!=0;err=[5,12002,0,0x80004005][i%4]
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def allocate(mu):oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')));return NEW if alloc_ok else 0
        def submit(mu):
            assert reg(mu,'EDX')==1;oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8'),{img.base+0xb800:0,img.base+0xc4a0:1}[reg(mu,'R9')]));return status
        def deliver(mu):oc.append(('deliver',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));return status
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base,reg(mu,'R8D')));return 0
        def read(mu):assert reg(mu,'R9')==0;oc.append(('read',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));mu.reg_write(X.UC_X86_REG_RAX,int(read_ok))
        def error(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,err)
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'WinHttpCloseHandle':lambda mu:log(mu,'close'),'WinHttpReadData':read,'GetLastError':error}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        args={'reserve_body_buffer':[E,OUT],'queue_read':[E],'schedule_read':[R,E],'deferred_read':[E,0],'publish_body':[E,[B0,B1][idx],length],'body_worker':[task,0]}[name]
        def setup(w):m.mu.mem_write(A,bytes(state))
        result,_,_,_=m.call(img.base+rva,args,'void' if restype is None else 'uint',setup=setup,fakes={img.base+0x932c:allocate,img.base+0x5c20:submit,img.base+0xab3c:deliver,img.base+0xa224:diag,img.base+0x5424:lambda mu:log(mu,'cancel')})
        assert result is None or isinstance(result,int),(name,i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return host(NEW) if alloc_ok else None
        @BSUBMIT
        def sub(q,t,k,c):nc.append(('submit',norm(q),norm(t),k));return status
        @DELIVER
        def dl(e,d,n,c):nc.append(('deliver',norm(e),norm(d),n));return status
        @DIAG
        def di(e,k,v,c):nc.append(('diag',norm(e),k,v))
        @READ
        def rd(h,b,n,c):nc.append(('read',norm(h),norm(b),n));return int(read_ok)
        @ERROR
        def er(c):nc.append(('error',));return err
        rs=Services(lock('enter'),lock('leave'),al,FREE(),lock('event'),lock('close'),LOCK(),ADJUST(),EVENT(),er,QUEUE(),di,DISPATCH(),rd,RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),lock('cancel'),SUBMIT(),None)
        bs=Body(C.pointer(rs),sub,dl)
        got=fn(*[host(v) if t==P else v for t,v in zip(argspec,args)],C.byref(bs));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(result,oc),(name,i,got,result,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 256',flush=True)
    print('XCurl body scheduling PASS:',total,'cases')
if __name__=='__main__':main()
