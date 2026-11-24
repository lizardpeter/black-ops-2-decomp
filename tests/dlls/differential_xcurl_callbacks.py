#!/usr/bin/env python3
from differential_xcurl_request import *
CB=C.CFUNCTYPE(Q,P,Q,Q,P); HSUB=C.CFUNCTYPE(U,P,P,U,P); HDIAG=C.CFUNCTYPE(None,P,U,Q,Q,P)
class Header(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('submit',HSUB),('diag',HDIAG)]

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0xb9b4)
    cases=[('invoke_write',0xb9b4,(CB,P,Q,P),Q),('invoke_read',0xc360,(P,P,Q,C.POINTER(Services)),Q),('queue_headers',0xbba8,(P,C.POINTER(Header)),U),('queue_upload',0xbf6c,(P,C.POINTER(Header)),U),('header_worker',0xb4c0,(P,C.POINTER(Header)),None)]
    CB1=A+0x5800;CB2=A+0x5900;USER1=0x13570000;USER2=0x13580000;total=0
    for name,rva,types,rt in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=types;fn.restype=rt
      for i in range(256):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set();mode=(i//4)%5
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v)
        raw=rng.choice([b'HTTP/1.1 200 OK\r\nX: test\r\n\r\n\0',b'A\nB\n\0',b'a\0',b'\0',b'A\r\n'+b'x'*80+b'\n\0'])
        state[BUF-A:BUF-A+len(raw)]=raw
        q(E+0x1d0,BUF);q(E+0x1c8,len(raw));q(E+0x130,CB1 if i&1 else 0);q(E+0x128,USER1);q(E+0x110,CB2 if i&2 else 0);q(E+0x108,USER2);q(E+0x120,CB1 if i&1 else 0);q(E+0x118,USER1)
        q(E+0x310,M if i&1 else 0);q(M+0xa0,EVENT_HANDLE+0x100);q(E+0x350,QUEUE_HANDLE);q(E+0x360,EVENT_HANDLE);q(R+0x30,E);q(R+0x50,HTTP);u(R+0x28,i%7);u(E+0x228,0x80004005 if i%11==0 else 0);u(E+0x220,77);state[E-A+0x175]=(i>>2)&1;state[E-A+0x176]=(i>>3)&1
        size=rng.choice([0,1,32,0x100000000,2**64-1]);status=[0,1,0x80004005,0xffffffff][i%4]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def callback_result(n):return [n,0x10000001,0,0xffffffffffffffff,(n+1)&0xffffffffffffffff][mode]
        def cb(mu,k):
            n=reg(mu,'R8');oc.append(('cb',k,reg(mu,'RCX'),reg(mu,'RDX'),n,reg(mu,'R9')));return callback_result(n)
        def submit(mu):
            assert reg(mu,'EDX')==1;oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8'),{img.base+0xb4c0:0,img.base+0xc9c0:1}[reg(mu,'R9')]));return status
        def release(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x20878 else reg(mu,'R8'),0 if site==0x20878 else reg(mu,'R9')));return 0
        def debug(mu):oc.append(('debug',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8'),reg(mu,'R9')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'SetEvent':lambda mu:log(mu,'event'),'WinHttpCloseHandle':lambda mu:log(mu,'close')}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        args={'invoke_write':[CB1 if i&1 else 0,BUF,size,USER1],'invoke_read':[E,BUF,size],'queue_headers':[E],'queue_upload':[E],'header_worker':[E,0]}[name]
        def setup(w):m.mu.mem_write(A,bytes(state));m.mu.mem_write(CB1,b'\xc3');m.mu.mem_write(CB2,b'\xc3')
        result,_,_,_=m.call(img.base+rva,args,'void' if rt is None else 'uint64' if rt==Q else 'uint',setup=setup,fakes={CB1:lambda mu:cb(mu,1),CB2:lambda mu:cb(mu,2),img.base+0x5c20:submit,img.base+0x9640:release,img.base+0xa224:diag,img.base+0xa128:debug,img.base+0x5424:lambda mu:log(mu,'cancel')})
        assert result is None or isinstance(result,int),(name,i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        def ncb(k):
            @CB
            def f(p,s,n,u):nc.append(('cb',k,norm(p),s,n,norm(u)));return callback_result(n)
            return f
        cb1,cb2=ncb(1),ncb(2);cbmap={CB1:C.cast(cb1,P).value,CB2:C.cast(cb2,P).value};rev={v:k for k,v in cbmap.items()}
        for off in offsets:
            v=struct.unpack_from('<Q',state,off)[0];struct.pack_into('<Q',native,off,cbmap.get(v,host(v)))
        # Callback code bytes are written only into the oracle's test memory.
        native[CB1-A]=native[CB2-A]=b'\xc3'
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @DEBUG
        def db(e,k,p,n,c):nc.append(('debug',norm(e),k,norm(p),n))
        @HSUB
        def sb(q,e,k,c):nc.append(('submit',norm(q),norm(e),k));return status
        @HDIAG
        def dg(e,k,a,b,c):nc.append(('diag',norm(e),k,a,b))
        rs=Services(lock('enter'),lock('leave'),ALLOC(),fr,lock('event'),lock('close'),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),db,lock('cancel'),SUBMIT(),None)
        hs=Header(C.pointer(rs),sb,dg)
        if name=='invoke_write':got=fn(cb1 if i&1 else CB(),host(BUF),size,USER1)
        elif name=='invoke_read':got=fn(host(E),host(BUF),size,C.byref(rs))
        else:got=fn(host(E),C.byref(hs))
        actual=bytearray(bytes(native))
        for off in offsets:
            v=struct.unpack_from('<Q',actual,off)[0];struct.pack_into('<Q',actual,off,rev.get(v,norm(v)))
        assert (got,nc)==(result,oc),(name,i,got,result,nc,oc)
        assert actual==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
      print(name,'PASS 256',flush=True)
    print('XCurl transfer callbacks PASS:',total,'cases')
if __name__=='__main__':main()
