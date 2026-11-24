#!/usr/bin/env python3
"""Original session cache and refcount; execute original alloc/free paths against a tagged heap model."""
from differential_xcurl_request import *
OPEN=C.CFUNCTYPE(P,P,U,P);OPTION=C.CFUNCTYPE(U,P,U,U,P)
class Session(C.Structure):
    _fields_=[('enter',LOCK),('leave',LOCK),('allocate',ALLOC),('release',FREE),('open',OPEN),('option',OPTION),('error',ERROR),('close',LOCK),('diag',DIAG),('vtable',P),('ctx',P)]
CACHE=A;ENTRY=[A+0x3000,A+0x3100,A+0x3200];OBJ=[A+0x3300,A+0x3400,A+0x3500];TEXT=[A+0x3600,A+0x3700,A+0x3800];AGENT=A+0x3900;NEW_ENTRY=A+0x3a00;NEW_OBJ=A+0x3b00;NEW_TEXT=A+0x3c00;OUT=A+0x3d00;H=0x11110000

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x15f40);total=0
    cases=[('session_acquire',0x15f40,(P,P,U,U,C.POINTER(P)),U,768),('session_cache_clear',0x15b64,(P,),None,256),('session_release',0x1ab90,(P,),U,256),('session_create',0x1a9d0,(P,C.POINTER(P)),U,256)]
    for name,rva,types,rt,count in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Session));fn.restype=rt
      for i in range(count):
        total+=1;state=bytearray(SIZE);offsets=set();oc=[];nc=[];oi=ni=oe=ne=oa=na=0
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        def text(a,s):d=s.encode('utf-16le')+b'\0\0';state[a-A:a-A+len(d)]=d
        def circle(h,nodes):
            chain=[h,*nodes]
            for k,p in enumerate(chain):q(p,chain[(k+1)%len(chain)]);q(p+8,chain[k-1])
        agent=rng.choice(['XCURL/2403.2.0.0.amd64fre','other','custom','']);custom=i%2==0;effective=agent if custom else 'XCURL/2403.2.0.0.amd64fre';tls=rng.randrange(3);scheme=rng.randrange(3);q(E+0xd8,AGENT if custom else 0);text(AGENT,agent)
        nodes=ENTRY[:i%4];circle(CACHE+0x28,nodes)
        for k in range(3):
            if ENTRY[k] not in nodes:circle(ENTRY[k],[])
            u(ENTRY[k]+0x10,rng.choice([tls,tls,7]));u(ENTRY[k]+0x20,rng.choice([scheme,scheme,7]));q(ENTRY[k]+0x18,TEXT[k]);q(ENTRY[k]+0x28,OBJ[k]);text(TEXT[k],rng.choice([effective,effective,'mismatch']))
            q(OBJ[k],img.base+0x1f058);u(OBJ[k]+8,rng.choice([0,1,2,3,0xffffffff]));q(OBJ[k]+0x10,H+k*0x100 if i%3 else 0)
        for addr,n in [(NEW_ENTRY,0x30),(NEW_OBJ,0x20),(NEW_TEXT,0x100)]:state[addr-A:addr-A+n]=rng.randbytes(n)
        offsets.update([NEW_ENTRY-A,NEW_ENTRY-A+8,NEW_ENTRY-A+0x18,NEW_ENTRY-A+0x28,NEW_OBJ-A,NEW_OBJ-A+0x10]);q(OUT,0x12345678)
        fail_tag=rng.choice([0,7,17,19]);opens=[rng.choice([0,H]),rng.choice([0,H])];errs=[rng.choice([5,87,12002,0x80004005,0xffffffff]) for _ in range(8)];option_ok=rng.randrange(2)
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def readwide(addr):
            out=b'';j=0
            while (v:=bytes(m.mu.mem_read(addr+j,2)))!=b'\0\0':out+=v;j+=2
            return out
        def open_(mu):
            nonlocal oi
            assert reg(mu,'EDX')==4 and reg(mu,'R8')==reg(mu,'R9')==0
            flags=struct.unpack('<I',mu.mem_read(reg(mu,'RSP')+0x28,4))[0];oc.append(('open',readwide(reg(mu,'RCX')),flags));mu.reg_write(X.UC_X86_REG_RAX,opens[oi]);oi+=1
        def option(mu):
            assert reg(mu,'R9D')==4;oc.append(('option',reg(mu,'RCX'),reg(mu,'EDX'),struct.unpack('<I',mu.mem_read(reg(mu,'R8'),4))[0]));mu.reg_write(X.UC_X86_REG_RAX,option_ok)
        def err(mu):
            nonlocal oe
            oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,errs[oe]);oe+=1
        def alloc(mu):
            n=reg(mu,'RCX');tag=reg(mu,'EDX');oc.append(('alloc',n,tag));return 0 if tag==fail_tag else {7:NEW_TEXT,17:NEW_OBJ,19:NEW_ENTRY}[tag]
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def diag(mu):oc.append(('diag',reg(mu,'RCX'),reg(mu,'RDX')-img.base,reg(mu,'R8D')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'WinHttpOpen':open_,'WinHttpSetOption':option,'GetLastError':err,'WinHttpCloseHandle':lambda mu:log(mu,'close')}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        def setup(w):m.mu.mem_write(A,bytes(state))
        args=[CACHE,E,tls,scheme,OUT] if name=='session_acquire' else [CACHE] if name=='session_cache_clear' else [OBJ[0]] if name=='session_release' else [H,OUT]
        ret,_,_,_=m.call(img.base+rva,args,'void' if rt is None else 'uint',setup=setup,fakes={img.base+0x932c:alloc,img.base+0x9640:free,img.base+0xa224:diag})
        assert ret is None or isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        for off in offsets:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return None if t==fail_tag else host({7:NEW_TEXT,17:NEW_OBJ,19:NEW_ENTRY}[t])
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @OPEN
        def op(p,flags,c):
            nonlocal ni
            out=b'';j=0
            while (v:=C.string_at(p+j,2))!=b'\0\0':out+=v;j+=2
            nc.append(('open',out,flags));r=opens[ni];ni+=1;return r or None
        @OPTION
        def so(p,k,v,c):nc.append(('option',norm(p),k,v));return option_ok
        @ERROR
        def er(c):
            nonlocal ne
            nc.append(('error',));r=errs[ne];ne+=1;return r
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k,v))
        services=Session(lock('enter'),lock('leave'),al,fr,op,so,er,lock('close'),di,img.base+0x1f058,None)
        native_args=[host(CACHE),host(E),tls,scheme,C.cast(host(OUT),C.POINTER(P))] if name=='session_acquire' else [host(CACHE)] if name=='session_cache_clear' else [host(OBJ[0])] if name=='session_release' else [H,C.cast(host(OUT),C.POINTER(P))]
        got=fn(*native_args,C.byref(services));raw=bytearray(bytes(native))
        for off in offsets:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,nc)==(ret,oc),(name,i,got,ret,nc,oc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS',count,flush=True)
    print('XCurl session cache PASS:',total,'cases')
if __name__=='__main__':main()
