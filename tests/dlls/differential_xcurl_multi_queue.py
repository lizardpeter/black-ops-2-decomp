#!/usr/bin/env python3
"""Original multi scheduling, intrusive lists and message lifecycle with explicit OS/engine boundaries."""
import ctypes as C
import hashlib,random,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/differential'))
from harness import Image,Machine
from differential_xcurl_getinfo import SHA256
from unicorn import x86_const as X
P=C.c_void_p;U=C.c_uint32
LOCK=C.CFUNCTYPE(None,P,P);WAIT=C.CFUNCTYPE(U,P,U,P);MANY=C.CFUNCTYPE(U,U,P,U,P)
CANCEL=C.CFUNCTYPE(None,P,U,U,P);DONE=C.CFUNCTYPE(C.c_uint8,P,P);ADJUST=C.CFUNCTYPE(None,U,P);FREE=C.CFUNCTYPE(None,P,U,P)
class Services(C.Structure):
    _fields_=[('enter',LOCK),('leave',LOCK),('wait',WAIT),('many',MANY),('release_mutex',LOCK),('event',LOCK),('advance_pending',LOCK),('advance_active',LOCK),('cancel',CANCEL),('after_cancel',LOCK),('progress',LOCK),('done',DONE),('adjust',ADJUST),('release',FREE),('ctx',P)]
A=0x34000000;E=A+0x1000;E2=A+0x2000;E3=A+0x3000;MSG=[A+0x4000,A+0x4100,A+0x4200];OUT=A+0x5000;SIZE=0x6000
MUTEX=0x12345678;EVENT1=0x12345680;EVENT2=0x12345688;EVENT3=0x12345690;EASY_EVENT=0x12345698

def main():
    dll,libpath=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(libpath.resolve()))
    functions={}
    for name,types,restype in [('multi_add',(P,P),U),('multi_remove',(P,P),U),('multi_perform_inner',(P,C.POINTER(U)),U),('multi_poll_inner',(P,U,C.POINTER(U)),U),('multi_info_read_inner',(P,C.POINTER(U)),P)]:
        fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Services));fn.restype=restype;functions[name]=fn
    rng=random.Random(0x18a28);total=0
    for action in ('add','remove','perform','poll','info'):
      for i in range(160):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);ptr_offsets=set();done_after=1+i%4;done_o=done_n=0
        def q(off,value):struct.pack_into('<Q',state,off,value);ptr_offsets.add(off)
        def u(off,value):struct.pack_into('<I',state,off,value&0xffffffff)
        def circle(head,nodes):
            chain=[head,*nodes]
            for k,p in enumerate(chain):q(p-A,chain[(k+1)%len(chain)]);q(p-A+8,chain[k-1])
        for off in (0,0x40,0x50,0x60,0x78):circle(A+off,[])
        for e in (E,E2,E3):circle(e,[]);q(e-A+0x310,0);q(e-A+0x360,EASY_EVENT);u(e-A+0x358,rng.getrandbits(32))
        q(0xa0,EVENT1);q(0xa8,EVENT2);q(0xb0,EVENT3);q(0xb8,MUTEX);q(0x70,0)
        u(0x38,rng.getrandbits(32));u(0x3c,[0,1,2,0xffffffff][i%4]);u(0x9c,rng.getrandbits(32));u(0x88,0);u(OUT-A,0xabcdef01)
        present=i%11!=0;ep=i%13!=0;out_present=i%7!=0;wait_result=[0,1,0x102,0xffffffff][i%4];timeout=rng.getrandbits(32)
        if action=='add':
            if i%3==0:q(E-A+0x310,A)
            circle(A+0x40,[E2,E3][:i%3])
        elif action=='remove':
            case=i%4
            if case<3:circle(A+[0x60,0x40,0x50][case],[E2,E,E3] if i%2 else [E]);q(E-A+0x310,A)
            else:circle(A+0x40,[E2,E3])
        elif action=='perform':circle(A+0x40,[E,E2,E3][:i%4])
        elif action=='info':
            nodes=MSG[:i%4];circle(A+0x78,nodes)
            prev=bool(nodes and i%2);q(0x70,nodes[0] if prev else 0);u(0x88,len(nodes)-int(prev))
            for node in MSG:
                u(node-A+0x10,rng.getrandbits(32));q(node-A+0x18,E);u(node-A+0x20,rng.getrandbits(32))
        for p in [A,A+0x40,A+0x50,A+0x60,A+0x78,E,E2,E3,*MSG]:ptr_offsets.update((p-A,p-A+8))
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def ol(mu,kind):oc.append((kind,reg(mu,'RCX')))
        def ow(mu):oc.append(('wait',reg(mu,'RCX'),reg(mu,'EDX')));mu.reg_write(X.UC_X86_REG_RAX,0)
        def om(mu):
            assert reg(mu,'R8D')==0;oc.append(('many',reg(mu,'ECX'),reg(mu,'RDX'),reg(mu,'R9D')));mu.reg_write(X.UC_X86_REG_RAX,wait_result)
        def event(mu,kind):oc.append((kind,reg(mu,'RCX')));mu.reg_write(X.UC_X86_REG_RAX,1)
        for name in img.iat.values():
            if name.endswith('!EnterCriticalSection'):m.on_import(name,lambda mu:ol(mu,'enter'))
            if name.endswith('!LeaveCriticalSection'):m.on_import(name,lambda mu:ol(mu,'leave'))
            if name.endswith('!WaitForSingleObject'):m.on_import(name,ow)
            if name.endswith('!WaitForMultipleObjects'):m.on_import(name,om)
            if name.endswith('!ReleaseMutex'):m.on_import(name,lambda mu:event(mu,'mutex'))
            if name.endswith('!SetEvent'):m.on_import(name,lambda mu:event(mu,'event'))
        def step(mu,kind):oc.append((kind,reg(mu,'RCX')));return 0
        def cancel(mu):oc.append(('cancel',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8D')));return 0
        def done(mu):
            nonlocal done_o;done_o+=1;oc.append(('done',reg(mu,'RCX')));return int(done_o>=done_after)
        def adjust(mu):oc.append(('adjust',reg(mu,'EDX')));return 0
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def setup(w):m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28c10,0x22222222)
        callbacks={img.base+0x18f68:lambda mu:step(mu,'pending'),img.base+0x19344:lambda mu:step(mu,'active'),img.base+0xca60:cancel,img.base+0xcefc:lambda mu:step(mu,'after_cancel'),img.base+0xa8f0:lambda mu:step(mu,'progress'),img.base+0xb6b4:done,img.base+0x16de4:adjust,img.base+0x9640:free}
        ma=A if present else 0;ea=E if ep else 0;oa=OUT if out_present or action=='info' else 0
        rva,args={'add':(0x19744,[ma,ea]),'remove':(0x19d20,[ma,ea]),'perform':(0x19b8c,[ma,oa]),'poll':(0x19c54,[ma,0,0,timeout,oa]),'info':(0x198e4,[ma,oa])}[action]
        ret,_,_,_=m.call(img.base+rva,args,'uint64' if action=='info' else 'uint',setup=setup,fakes=callbacks)
        assert isinstance(ret,int),(action,i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE))
        native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in ptr_offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        @LOCK
        def en(p,c):nc.append(('enter',norm(p)))
        @LOCK
        def le(p,c):nc.append(('leave',norm(p)))
        @WAIT
        def wait(p,t,c):nc.append(('wait',norm(p),t));return 0
        @MANY
        def many(n,p,t,c):nc.append(('many',n,norm(p),t));return wait_result
        @LOCK
        def mutex(p,c):nc.append(('mutex',norm(p)))
        @LOCK
        def ev(p,c):nc.append(('event',norm(p)))
        @LOCK
        def pending(p,c):nc.append(('pending',norm(p)))
        @LOCK
        def active(p,c):nc.append(('active',norm(p)))
        @CANCEL
        def can(p,h,r,c):nc.append(('cancel',norm(p),h,r))
        @LOCK
        def after(p,c):nc.append(('after_cancel',norm(p)))
        @LOCK
        def progress(p,c):nc.append(('progress',norm(p)))
        @DONE
        def dn(p,c):
            nonlocal done_n;done_n+=1;nc.append(('done',norm(p)));return int(done_n>=done_after)
        @ADJUST
        def ad(n,c):nc.append(('adjust',n))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        services=Services(en,le,wait,many,mutex,ev,pending,active,can,after,progress,dn,ad,fr,None)
        mp=host(ma);eptr=host(ea);op=C.cast(host(oa),C.POINTER(U)) if oa else None
        if action=='add':got=functions['multi_add'](mp,eptr,C.byref(services))
        elif action=='remove':got=functions['multi_remove'](mp,eptr,C.byref(services))
        elif action=='perform':got=functions['multi_perform_inner'](mp,op,C.byref(services))
        elif action=='poll':got=functions['multi_poll_inner'](mp,timeout,op,C.byref(services))
        else:got=norm(functions['multi_info_read_inner'](mp,op,C.byref(services)))
        raw=bytearray(bytes(native))
        for off in ptr_offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert got==ret and oc==nc,(action,i,got,ret,oc,nc)
        assert raw==expected,(action,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:50])
    print(f'XCurl multi scheduling PASS: {total} original-machine cases (add/remove/perform/poll/info)')
if __name__=='__main__':main()
