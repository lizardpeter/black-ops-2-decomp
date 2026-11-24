#!/usr/bin/env python3
"""Original scheduler, reservation limits, generation accounting and wakeup."""
from differential_xcurl_multi_queue import *
G=A+0x4400
class Scheduler(C.Structure):
    _fields_=[('enter',LOCK),('leave',LOCK),('start',LOCK),('progress',LOCK),('cancel',CANCEL),('after',LOCK),('reset',LOCK),('event',LOCK),('wait',WAIT),('ctx',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x18f68);total=0
    cases=[('advance_pending',0x18f68,(P,P),None),('advance_active',0x19344,(P,),None),('abort_all',0x1925c,(P,),None),('multi_wait',0x19514,(P,U,C.POINTER(U)),U),('multi_wakeup',0x19614,(P,),U)]
    for name,rva,types,rt in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Scheduler));fn.restype=rt
      for i in range(256):
        total+=1;state=bytearray(SIZE);offsets=set();oc=[];nc=[]
        def q(p,v):struct.pack_into('<Q',state,p-A,v);offsets.add(p-A)
        def u(p,v):struct.pack_into('<I',state,p-A,v&0xffffffff)
        def circle(h,nodes):
            chain=[h,*nodes]
            for k,p in enumerate(chain):q(p,chain[(k+1)%len(chain)]);q(p+8,chain[k-1])
        nodes=[E,E2,E3];rng.shuffle(nodes);split=rng.randrange(4);pending=nodes[:split];active=nodes[split:split+rng.randrange(4-split)]
        for e in nodes:circle(e,[])
        circle(A+0x40,pending);circle(A+0x50,active)
        q(A+0xa0,EVENT1);q(A+0xa8,EVENT2);q(A+0xb0,EVENT3)
        u(A+0x3c,rng.getrandbits(32));state[0xc0]=rng.randrange(2);epoch=rng.choice([0,1,0xffffffff,rng.getrandbits(32)]);u(A+0xc4,epoch)
        for e in nodes:u(e+0x358,rng.choice([0,epoch,(epoch+1)&0xffffffff,rng.getrandbits(32)]))
        u(A+0x98,rng.choice([0,1,2,0xffffffff]));u(A+0x9c,rng.choice([0,1,2,0xffffffff]));u(OUT,0xabcdef01)
        u(G+0x40,rng.choice([0,1,2,3,4,0xfffffffe,0xffffffff]));u(G+0x44,rng.choice([0,1,2,3,4,0xffffffff]));state[G-A+0x49]=rng.randrange(2)
        # Model a concurrent abort between reservation and admission.
        race=rng.randrange(4) if name=='advance_pending' else 0;ent_o=ent_n=0
        timeout=rng.getrandbits(32);wait_result=rng.choice([0,1,0x102,0xffffffff]);has_out=bool(i%2)
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def enter(mu):
            nonlocal ent_o
            p=reg(mu,'RCX');oc.append(('enter',p))
            if p==A+0x10:
                ent_o+=1
                if ent_o==2 and race==1:mu.mem_write(A+0xc0,b'\1')
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def wait(mu):oc.append(('wait',reg(mu,'RCX'),reg(mu,'EDX')));mu.reg_write(X.UC_X86_REG_RAX,wait_result)
        def cancel(mu):oc.append(('cancel',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D')));return 0
        for n in img.iat.values():
            k=n.split('!')[-1];f={'EnterCriticalSection':enter,'LeaveCriticalSection':lambda mu:log(mu,'leave'),'ResetEvent':lambda mu:log(mu,'reset'),'SetEvent':lambda mu:log(mu,'event'),'WaitForSingleObject':wait}.get(k)
            if f:m.on_import(n,f)
        def setup(w):m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,G)
        args=[A,timeout,OUT if has_out else 0] if name=='multi_wait' else [A]
        ret,_,_,_=m.call(img.base+rva,args,'void' if rt is None else 'uint',setup=setup,fakes={img.base+0xcf90:lambda mu:log(mu,'start'),img.base+0xa8f0:lambda mu:log(mu,'progress'),img.base+0xca60:cancel,img.base+0xcefc:lambda mu:log(mu,'after')})
        assert ret is None or isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        @LOCK
        def en(p,c):
            nonlocal ent_n
            p=norm(p);nc.append(('enter',p))
            if p==A+0x10:
                ent_n+=1
                if ent_n==2 and race==1:native[0xc0]=b'\1'
        def logn(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @CANCEL
        def can(p,h,r,c):nc.append(('cancel',norm(p),h,r))
        @WAIT
        def wa(p,t,c):nc.append(('wait',norm(p),t));return wait_result
        services=Scheduler(en,logn('leave'),logn('start'),logn('progress'),can,logn('after'),logn('reset'),logn('event'),wa,None)
        nargs=[base,host(G)] if name=='advance_pending' else [base,timeout,C.cast(host(OUT),C.POINTER(U)) if has_out else None] if name=='multi_wait' else [base]
        got=fn(*nargs,C.byref(services));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert got==ret and oc==nc,(name,i,got,ret,oc,nc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:40])
      print(name,'PASS 256',flush=True)
    print('XCurl scheduler PASS:',total,'cases')
if __name__=='__main__':main()
