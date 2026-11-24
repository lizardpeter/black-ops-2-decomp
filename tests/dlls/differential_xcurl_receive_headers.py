#!/usr/bin/env python3
from differential_xcurl_header_query import *
CRACK=C.CFUNCTYPE(U,P,P,C.POINTER(U),P);DIAGNOSTIC=C.CFUNCTYPE(None,P,U,Q,U,P)
class Receive(C.Structure):
    _fields_=[('query',C.POINTER(QueryServices)),('request',C.POINTER(Services)),('crack',CRACK),('diag',DIAGNOSTIC)]
ARENA=0xa000;URL=A+0x3000;CODE=A+0x80;BUFFERS=[A+0x6000+i*0x400 for i in range(5)]

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x12228);fn=lib.xcurl_research_receive_headers;fn.argtypes=(P,P,C.POINTER(U),C.POINTER(Receive));fn.restype=U
    for i in range(1024):
        oc=[];nc=[];state=bytearray(ARENA);last_o=last_n=0;oa=na=0
        def q(p,v):struct.pack_into('<Q',state,p-A,v)
        def u(p,v):struct.pack_into('<I',state,p-A,v)
        q(R+0x30,E);q(R+0x50,HTTP);q(R+0x38,URL);q(R+0xc0,rng.getrandbits(64));state[R-A+0xb8]=rng.randrange(2);u(CODE,0x12345678)
        url=rng.choice(['https://example.test/abc','http://example.test/é','http://localhost/','https://example.test/你好']);urlraw=url.encode('utf-16le')+b'\0\0';state[URL-A:URL-A+len(urlraw)]=urlraw
        raw=rng.choice(['HTTP/1.0','HTTP/1.1','HTTP/2','HTTP/3','X'])+' 200 OK\r\nX: test\r\n\r\n';ctype=rng.choice(['text/plain','text/html; charset=UTF-8','é','']);text={0x16:raw.encode('utf-16le')+b'\0\0',1:ctype.encode('utf-16le')+b'\0\0'}
        numbers={0x20000013:rng.randrange(100,600),0x20000005:rng.choice([0,1,100,0xffffffff]),0x20000024:rng.getrandbits(32)};numok={x:(rng.randrange(4)!=0) for x in numbers};numerr=rng.choice([0,5,12002,0x80004005]);first={x:rng.choice([122,122,122,0,5,0x80004005]) for x in text};second={x:rng.choice([0,5,122,0xffffffff]) for x in text};secondok={x:rng.randrange(4)!=0 for x in text};fail=rng.randrange(7);crack_status=rng.choice([0,0,1,0x80004005]);scheme=rng.randrange(4)
        for p in BUFFERS:state[p-A:p-A+0x400]=b'\xa5'*0x400
        def query_model(trace,h,level,dst,cap,write_data,write_size):
            trace.append(('query',h,level,'local' if level in (0x20000005,0x20000024) else dst,cap))
            if level in numbers:
                if numok[level]:write_data(struct.pack('<I',numbers[level]));return 1,0
                return 0,numerr
            raw=text[level]
            if not dst:write_size(len(raw));return 0,first[level]
            if secondok[level]:write_data(raw);write_size(len(raw)-2);return 1,0
            return 0,second[level]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def oq(mu):
            nonlocal last_o
            h,level,unused,dst=[reg(mu,k) for k in ('RCX','EDX','R8','R9')];assert unused==0;p,idx=struct.unpack('<QQ',mu.mem_read(reg(mu,'RSP')+0x28,16));assert idx==0
            cap=struct.unpack('<I',mu.mem_read(p,4))[0];result,last_o=query_model(oc,h,level,dst,cap,lambda data:mu.mem_write(dst,data),lambda n:mu.mem_write(p,struct.pack('<I',n)));mu.reg_write(X.UC_X86_REG_RAX,result)
        def oe(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,last_o)
        def alloc(mu):
            nonlocal oa
            n,t=reg(mu,'RCX'),reg(mu,'EDX');assert n<=0x400;oc.append(('alloc',n,t));oa+=1;return 0 if oa==fail else BUFFERS[oa-1]
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def convert(mu):
            assert reg(mu,'ECX')==65001 and reg(mu,'EDX')==0x80;p,n=reg(mu,'R8'),reg(mu,'R9D');d,cap,x,y=struct.unpack('<QQQQ',mu.mem_read(reg(mu,'RSP')+0x28,32));assert x==y==0
            encoded=bytes(mu.mem_read(p,n*2)).decode('utf-16le').encode('utf-8') if n else b'';oc.append(('convert',p,n,d,cap&0xffffffff))
            if d:mu.mem_write(d,encoded)
            mu.reg_write(X.UC_X86_REG_RAX,len(encoded))
        def crack(mu):oc.append(('crack',reg(mu,'RCX'),reg(mu,'RDX')));mu.mem_write(reg(mu,'R8')+0x14,struct.pack('<I',scheme));return crack_status
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,reg(mu,'R8') if site==0x216d0 else reg(mu,'R8D'),reg(mu,'R9D') if site==0x216d0 else 0));return 0
        def lock(mu,k):oc.append((k,reg(mu,'RCX')))
        def strncmp(mu):
            a,b,n=[reg(mu,k) for k in ('RCX','RDX','R8')];x=bytes(mu.mem_read(a,n)).split(b'\0',1)[0];y=bytes(mu.mem_read(b,n)).split(b'\0',1)[0];mu.reg_write(X.UC_X86_REG_RAX,(int(x>y)-int(x<y))&0xffffffff)
        for n in img.iat.values():
            f={'WinHttpQueryHeaders':oq,'GetLastError':oe,'WideCharToMultiByte':convert,'strncmp':strncmp,'EnterCriticalSection':lambda mu:lock(mu,'enter'),'LeaveCriticalSection':lambda mu:lock(mu,'leave')}.get(n.split('!')[-1]);
            if f:m.on_import(n,f)
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+0x12228,[R,E,CODE],'uint',setup=setup,fakes={img.base+0x932c:alloc,img.base+0x9640:free,img.base+0x14ac0:crack,img.base+0xa224:diag});assert isinstance(ret,int),(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,ARENA));native=C.create_string_buffer(bytes(state),ARENA);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+ARENA else v or 0
        def host(v):return base+v-A if A<=v<A+ARENA else v
        for p in (R+0x30,R+0x38):struct.pack_into('<Q',native,p-A,host(struct.unpack_from('<Q',state,p-A)[0]))
        @ALLOC
        def al(n,t,c):
            nonlocal na
            nc.append(('alloc',n,t));na+=1;return None if na==fail else host(BUFFERS[na-1])
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @QUERY
        def query(h,l,d,sz,c):
            nonlocal last_n
            result,last_n=query_model(nc,norm(h),l,norm(d),sz[0],lambda data:C.memmove(d,data,len(data)),lambda n:sz.__setitem__(0,n));return result
        @ERROR
        def err(c):nc.append(('error',));return last_n
        @CONVERT
        def co(p,n,d,cap,c):
            encoded=C.string_at(p,n*2).decode('utf-16le').encode('utf-8') if n else b'';nc.append(('convert',norm(p),n,norm(d),cap))
            if d:C.memmove(d,encoded,len(encoded))
            return len(encoded)
        @CRACK
        def cr(p,e,sc,c):nc.append(('crack',norm(p),norm(e)));sc[0]=scheme;return crack_status
        @DIAGNOSTIC
        def di(e,k,v,z,c):nc.append(('diag',norm(e),k,norm(v),z))
        def lk(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        rs=Services(lk('enter'),lk('leave'),ALLOC(),FREE(),LOCK(),LOCK(),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),LOCK(),SUBMIT(),None)
        qs=QueryServices(al,fr,query,err,co,None);svc=Receive(C.pointer(qs),C.pointer(rs),cr,di)
        got=fn(host(R),host(E),C.cast(host(CODE),C.POINTER(U)),C.byref(svc));actual=bytearray(bytes(native))
        for p in (R+0x30,R+0x38,E+0x208,E+0x1d0,E+0x218):struct.pack_into('<Q',actual,p-A,norm(struct.unpack_from('<Q',actual,p-A)[0]))
        sp=struct.unpack_from('<Q',actual,E-A+0x210)[0]
        if sp:struct.pack_into('<Q',actual,E-A+0x210,img.base+(0x20658 if C.string_at(sp)==b'http' else 0x20660))
        assert (got,nc)==(ret,oc),(i,got,ret,nc,oc)
        assert actual==expected,(i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(actual,expected)) if a!=b][:30])
    print('XCurl full response header orchestration PASS: 1024 cases')
if __name__=='__main__':main()
