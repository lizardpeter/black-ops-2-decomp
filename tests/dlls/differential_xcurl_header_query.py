#!/usr/bin/env python3
from differential_xcurl_request import *
QUERY=C.CFUNCTYPE(U,P,U,P,C.POINTER(U),P);CONVERT=C.CFUNCTYPE(C.c_int,P,C.c_int,P,C.c_int,P)
class QueryServices(C.Structure):
    _fields_=[('alloc',ALLOC),('free',FREE),('query',QUERY),('err',ERROR),('convert',CONVERT),('ctx',P)]
OUT=A+0x100;LEN=A+0x108;WIDE=A+0x3000;NARROW=A+0x4000;TEXT=A+0x5000

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x14d04);total=0
    cases=[('query_wide',0x14d04,(P,U,C.POINTER(P),C.POINTER(Q))),('header_utf8',0x1abb0,(P,C.POINTER(P),C.POINTER(Q))),('query_utf8',0x14f84,(P,U,C.POINTER(P),C.POINTER(Q)))]
    for name,rva,types in cases:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(QueryServices));fn.restype=U
      for i in range(512):
        total+=1;state=bytearray(SIZE);oc=[];nc=[];oi=ni=oe=ne=oa=na=0
        text=rng.choice(['','HTTP/1.1 200 OK\r\n','é','你好','😀','a😀b','X: é\r\n\r\n']);raw=text.encode('utf-16le')+b'\0\0';state[TEXT-A:TEXT-A+len(raw)]=raw
        for off in [WIDE-A,NARROW-A]:state[off:off+256]=b'\xa5'*256
        struct.pack_into('<QQ',state,OUT-A,0x12345678,0x123456789abcdef0)
        first=rng.choice([122,122,122,0,5,0x80004005]);second=rng.choice([0,5,122,0xffffffff]);second_success=i%4!=0;first_success=i%3==0;fail=rng.choice([0,1,2]);level=rng.getrandbits(32);required=len(raw);written=len(raw)-2
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def q(mu):
            nonlocal oi
            h,lv,nm,dst=map(lambda k:reg(mu,k),('RCX','EDX','R8','R9'));assert nm==0
            p,idx=struct.unpack('<QQ',mu.mem_read(reg(mu,'RSP')+0x28,16));assert idx==0;cap=struct.unpack('<I',mu.mem_read(p,4))[0]
            oc.append(('query',h,lv,dst,cap));oi+=1
            if dst:mu.mem_write(dst,raw);mu.mem_write(p,struct.pack('<I',written));v=second_success
            else:mu.mem_write(p,struct.pack('<I',required));v=first_success
            mu.reg_write(X.UC_X86_REG_RAX,int(v))
        def er(mu):
            nonlocal oe
            oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,[first,second][oe]);oe+=1
        def al(mu):
            nonlocal oa
            n,t=reg(mu,'RCX'),reg(mu,'EDX');oc.append(('alloc',n,t));oa+=1
            return 0 if oa==fail else NARROW if name=='header_utf8' or oa==2 else WIDE
        def fr(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def conv(mu):
            assert reg(mu,'ECX')==65001 and reg(mu,'EDX')==0x80
            p,n=reg(mu,'R8'),reg(mu,'R9D');d,cap,x,y=struct.unpack('<QQQQ',mu.mem_read(reg(mu,'RSP')+0x28,32));assert x==y==0
            encoded=bytes(mu.mem_read(p,n*2)).decode('utf-16le').encode('utf-8') if n else b''
            oc.append(('convert',p,n,d,cap&0xffffffff));
            if d:mu.mem_write(d,encoded)
            mu.reg_write(X.UC_X86_REG_RAX,len(encoded))
        for n in img.iat.values():
            f={'WinHttpQueryHeaders':q,'GetLastError':er,'WideCharToMultiByte':conv}.get(n.split('!')[-1])
            if f:m.on_import(n,f)
        args=[TEXT,OUT,LEN] if name=='header_utf8' else [HTTP,level,OUT,LEN]
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+rva,args,'uint',setup=setup,fakes={img.base+0x932c:al,img.base+0x9640:fr});assert isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        @ALLOC
        def a(n,t,c):
            nonlocal na
            nc.append(('alloc',n,t));na+=1;return None if na==fail else host(NARROW if name=='header_utf8' or na==2 else WIDE)
        @FREE
        def f(p,t,c):nc.append(('free',norm(p),t))
        @QUERY
        def query(h,lv,dst,sz,c):
            nc.append(('query',norm(h),lv,norm(dst),sz[0]))
            if dst:C.memmove(dst,raw,len(raw));sz[0]=written;return int(second_success)
            sz[0]=required;return int(first_success)
        @ERROR
        def err(c):
            nonlocal ne
            nc.append(('error',));v=[first,second][ne];ne+=1;return v
        @CONVERT
        def convert(p,n,d,cap,c):
            encoded=C.string_at(p,n*2).decode('utf-16le').encode('utf-8') if n else b'';nc.append(('convert',norm(p),n,norm(d),cap))
            if d:C.memmove(d,encoded,len(encoded))
            return len(encoded)
        svc=QueryServices(a,f,query,err,convert,None)
        out=C.cast(host(OUT),C.POINTER(P));sz=C.cast(host(LEN),C.POINTER(Q));got=fn(host(TEXT),out,sz,C.byref(svc)) if name=='header_utf8' else fn(HTTP,level,out,sz,C.byref(svc))
        actual=bytearray(bytes(native));struct.pack_into('<Q',actual,OUT-A,norm(out[0]))
        assert (got,nc)==(ret,oc),(name,i,got,ret,nc,oc)
        assert actual==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
      print(name,'PASS 512',flush=True)
    fn=lib.xcurl_research_publish_header_fields;fn.argtypes=(P,C.POINTER(P),U,B,Q,Q,C.POINTER(P),U,P,U,C.POINTER(P),U,C.POINTER(Services));fn.restype=None
    for i in range(512):
        total+=1;state=bytearray(rng.randbytes(SIZE));oc=[];nc=[];pointers=[rng.choice([0,0x12340000+j*0x100]) for j in range(3)];slots=[OUT,OUT+8,OUT+16]
        for slot,p in zip(slots,pointers):struct.pack_into('<Q',state,slot-A,p)
        code=rng.getrandbits(32);known=rng.getrandbits(8);content=rng.getrandbits(64);header=rng.getrandbits(64);version=rng.getrandbits(32);scheme=rng.getrandbits(64);protocol=rng.getrandbits(32);extra=rng.getrandbits(32)
        def log(mu,k):oc.append((k,mu.reg_read(X.UC_X86_REG_RCX)))
        for n in img.iat.values():
            k=n.split('!')[-1]
            if k in ('EnterCriticalSection','LeaveCriticalSection'):m.on_import(n,lambda mu,k=k:log(mu,k))
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+0xcd4c,[E,slots[0],code,known,content,header,slots[1],version,scheme,protocol,slots[2],extra],'void',setup=setup);assert ret is None,ret
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def lock(k):return LOCK(lambda p,c:nc.append((k,A+p-base)))
        rs=Services(lock('EnterCriticalSection'),lock('LeaveCriticalSection'),ALLOC(),FREE(),LOCK(),LOCK(),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),LOCK(),SUBMIT(),None)
        ps=[C.cast(base+s-A,C.POINTER(P)) for s in slots]
        fn(base+E-A,ps[0],code,known,content,header,ps[1],version,scheme,protocol,ps[2],extra,C.byref(rs))
        assert oc==nc,(i,oc,nc);assert bytes(native)==expected,(i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(bytes(native),expected)) if a!=b][:30])
    print('publish_header_fields PASS 512',flush=True);print('XCurl header query PASS:',total,'cases')
if __name__=='__main__':main()
