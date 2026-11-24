#!/usr/bin/env python3
"""Original asynchronous request startup; explicitly model GDK result and setup service boundaries."""
from differential_xcurl_upload_buffer import *
STATUS=C.CFUNCTYPE(U,P,P);THREE=C.CFUNCTYPE(U,P,P,P,P);TWO=C.CFUNCTYPE(U,P,P,P);OPTIONS=C.CFUNCTYPE(U,P,P,U,P)
QUERYFN=C.CFUNCTYPE(U,P,P,C.POINTER(P),P)
class Gdk(C.Structure):
    _fields_=[('query',QUERYFN),('context',P)]
class Start(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('buffer',C.POINTER(Buffer)),('gdk',C.POINTER(Gdk)),('crack',THREE),('open',THREE),('register',STATUS),('options',OPTIONS),('headers',TWO),('certificate',TWO),('diag',DIAG)]

def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x11978);total=0
    TASK=A+0x3000;META=A+0x3100;ITEM=A+0x3200;BODY=A+0x4000;DATA=A+0x4500;CB1=A+0x5900;URL=A+0x5000;USER=0x12345678
    ASYNC_IFACE=A+0x5200;NET_IFACE=A+0x5300;ASYNC_VT=A+0x5400;NET_VT=A+0x5500;QUERY=A+0x5a00;STATUS_METHOD=A+0x5b00;SIZE_METHOD=A+0x5c00;RESULT_METHOD=A+0x5d00;RELEASE=A+0x5e00
    for name,rva,argspec in [('start_request',0x11978,(P,P)),('start_worker',0x11f10,(P,))]:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*argspec,C.POINTER(Start));fn.restype=None
      for i in range(512):
        total+=1;state=bytearray(SIZE);oc=[];nc=[];offsets=set();state_value=1 if i%7 else i%7+2;method=rng.randrange(4);mode=rng.randrange(4);block=64;fixed=rng.choice([0,32,64,96]);post=rng.choice([0,5,64,0xffffffffffffffff]);immediate=i%3==0;has_cb=i%3!=1;allocate_fail=i%13==0;phase=rng.randrange(15);error=0x80004005;statuses=[error if j==phase else 0 for j in range(8)];scheme=rng.choice([1,2]);itemtypes=[rng.randrange(4) for _ in range(rng.randrange(4))];code=0x80004005 if i%9==0 else 0;seq=[rng.choice([0,1,5]),0];oi=ni=0
        def q(a,v):struct.pack_into('<Q',state,a-A,v);offsets.add(a-A)
        def u(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        q(TASK+8,R);q(ASYNC_IFACE,ASYNC_VT);q(NET_IFACE,NET_VT);q(ASYNC_VT+2*8,RELEASE);q(ASYNC_VT+3*8,STATUS_METHOD);q(NET_VT+2*8,RELEASE);q(NET_VT+12*8,SIZE_METHOD);q(NET_VT+13*8,RESULT_METHOD);q(R+0x30,E);q(R+0x38,URL);q(R+0x50,HTTP);q(R+0x90,META if i%2 else 0);q(R+0x98,0);q(R+0xb0,0);q(R+0xa8,0);u(R+0x28,state_value);u(R+0xd0,mode);state[R-A+0xd6]=i%2;state[R-A+0xd5]=i%2
        q(E+0x310,M);q(M+0xa0,EVENT_HANDLE+0x100);q(E+0x350,QUEUE_HANDLE);q(E+0x360,EVENT_HANDLE);q(E+0x120,CB1 if has_cb else 0);q(E+0x118,USER);q(E+0x98,0x12340000);q(E+0xd0,0x12350000);q(E+0xc0,DATA if immediate else 0);q(E+0xf0,fixed);q(E+0xf8,post);u(E+0xb0,block);u(E+0x80,method);u(E+0x228,code);u(E+0x220,77);state[E-A+0x176]=i%2
        q(META+8,len(itemtypes));q(META+0x10,ITEM)
        for j,v in enumerate(itemtypes):u(ITEM+j*0x18,v)
        state[DATA-A:DATA-A+6]=b'hello\0';state[CB1-A]=0xc3
        # Fixed initial read must not exceed the configured capacity in the original-valid domain.
        limit=min(block,post if method==2 and post not in (0,0xffffffffffffffff) else fixed);seq[0]=min(seq[0],limit) if mode==1 else seq[0]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def log(mu,k):oc.append((k,reg(mu,'RCX')));return 0
        def alloc(mu):
            n,t=reg(mu,'RCX'),reg(mu,'EDX');oc.append(('alloc',n,t));return 0 if allocate_fail else META if t==15 else BODY
        def realloc(mu):oc.append(('realloc',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));return BODY
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        async_guid=bytes(m.mu.mem_read(img.base+0x20120,16))
        def query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16));oc.append(('query',cls,iid))
            iface=ASYNC_IFACE if cls==async_guid else NET_IFACE
            mu.mem_write(reg(mu,'R8'),struct.pack('<Q',iface));mu.reg_write(X.UC_X86_REG_RAX,0)
        def async_status(mu):
            oc.append(('async_status',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8')&0xff));mu.reg_write(X.UC_X86_REG_RAX,statuses[0])
        def network_size(mu):
            oc.append(('network_size',reg(mu,'RCX'),reg(mu,'RDX')));mu.mem_write(reg(mu,'R8'),struct.pack('<Q',64));mu.reg_write(X.UC_X86_REG_RAX,statuses[1])
        def network_result(mu):
            rsp=reg(mu,'RSP');buffer=struct.unpack('<Q',mu.mem_read(rsp+0x28,8))[0];out=struct.unpack('<Q',mu.mem_read(rsp+0x30,8))[0]
            oc.append(('network_result',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8'),reg(mu,'R9'),buffer,out))
            mu.mem_write(out,struct.pack('<Q',META));mu.reg_write(X.UC_X86_REG_RAX,statuses[2])
        def release_interface(mu):oc.append(('release',reg(mu,'RCX')));mu.reg_write(X.UC_X86_REG_RAX,1)
        def crack(mu):oc.append(('crack',reg(mu,'RCX'),reg(mu,'RDX')));mu.mem_write(reg(mu,'R8')+0x14,struct.pack('<I',scheme));return statuses[3]
        def op(mu):oc.append(('open',reg(mu,'RCX'),reg(mu,'RDX'),struct.unpack('<I',mu.mem_read(reg(mu,'R8')+0x14,4))[0]));return statuses[4]
        def register(mu):oc.append(('register',reg(mu,'RCX')));return statuses[5]
        def opts(mu):oc.append(('options',reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8D')));return statuses[6]
        def headers(mu):oc.append(('headers',reg(mu,'RCX'),reg(mu,'RDX')));return statuses[7]
        def certificate(mu):oc.append(('certificate',reg(mu,'RCX'),reg(mu,'RDX')));return error if phase==8 else 0
        def cb(mu):
            nonlocal oi
            p,s,n,ud=[reg(mu,k) for k in ('RCX','RDX','R8','R9')];oc.append(('read_cb',p,s,n,ud));v=seq[oi];oi+=1;mu.mem_write(p,b'z'*v);return v
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x21038 else reg(mu,'R8D')));return 0
        def debug(mu):oc.append(('debug',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8'),reg(mu,'R9')));return 0
        def send(mu):
            assert reg(mu,'RDX')==reg(mu,'R8')==0;ss,tt,ctx=struct.unpack('<QQQ',mu.mem_read(reg(mu,'RSP')+0x28,24));oc.append(('send',reg(mu,'RCX'),reg(mu,'R9'),ss&0xffffffff,tt&0xffffffff,ctx));mu.reg_write(X.UC_X86_REG_RAX,int(phase!=9))
        def err(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,5)
        def sub(mu):oc.append(('submit',reg(mu,'RCX'),reg(mu,'R8')));return 0
        imports={'EnterCriticalSection':lambda mu:log(mu,'enter'),'LeaveCriticalSection':lambda mu:log(mu,'leave'),'WinHttpCloseHandle':lambda mu:log(mu,'close'),'WinHttpSendRequest':send,'SetEvent':lambda mu:log(mu,'event'),'GetLastError':err}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        def setup(w):
            m.mu.mem_write(A,bytes(state))
            m.mu.mem_write(img.base+0x289d0,bytes([1,0]))
            m.mu.mem_write(img.base+0x289d8,struct.pack('<Q',0x12345678))
            m.mu.mem_write(img.base+0x289e0,struct.pack('<Q',QUERY))
        fakes={img.base+k:v for k,v in {0x932c:alloc,0x96f4:realloc,0x9640:free,0x14ac0:crack,0x137b8:op,0x13038:register,0x131a8:opts,0x10038:headers,0x11478:certificate,0xa224:diag,0xa128:debug,0x5c20:sub,0x5424:lambda mu:log(mu,'cancel')}.items()};fakes.update({QUERY:query,STATUS_METHOD:async_status,SIZE_METHOD:network_size,RESULT_METHOD:network_result,RELEASE:release_interface});fakes[CB1]=cb
        ret,_,_,_=m.call(img.base+rva,[R,TASK] if name=='start_request' else [TASK,0],'void',setup=setup,fakes=fakes);assert ret is None,(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A if A<=v<A+SIZE else v
        @CB
        def ncb(p,s,n,ud):
            nonlocal ni
            nc.append(('read_cb',norm(p),s,n,norm(ud)));v=seq[ni];ni+=1;C.memmove(p,b'z'*v,v);return v
        ncbptr=C.cast(ncb,P).value
        for off in offsets:
            v=struct.unpack_from('<Q',state,off)[0];struct.pack_into('<Q',native,off,ncbptr if v==CB1 else host(v))
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @ALLOC
        def al(n,t,c):nc.append(('alloc',n,t));return None if allocate_fail else host(META if t==15 else BODY)
        @REALLOC
        def re(p,n,t,c):nc.append(('realloc',norm(p),n,t));return host(BODY)
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        @DIAG
        def di(p,k,v,c):nc.append(('diag',norm(p),k,v))
        @UD
        def du(p,k,v,z,c):nc.append(('diag',norm(p),k,v))
        @DEBUG
        def db(p,k,d,n,c):nc.append(('debug',norm(p),k,norm(d),n))
        @SEND
        def sn(h,p,n,t,x,c):nc.append(('send',norm(h),norm(p),n,t,norm(x)));return int(phase!=9)
        @ERROR
        def er(c):nc.append(('error',));return 5
        @SUBMIT
        def sb(q,e,c):nc.append(('submit',norm(q),norm(e)));return 0
        rs=Services(lock('enter'),lock('leave'),al,fr,lock('event'),lock('close'),LOCK(),ADJUST(),EVENT(),er,QUEUE(),di,DISPATCH(),READ(),RECEIVE(),sn,CONTEXT(),CALLBACK(),db,lock('cancel'),sb,None);bs=Buffer(C.pointer(rs),re,du)
        def logical_iface(p):
            if p==C.addressof(async_iface):return ASYNC_IFACE
            if p==C.addressof(net_iface):return NET_IFACE
            return norm(p)
        @C.CFUNCTYPE(U,P)
        def rel(p):nc.append(('release',logical_iface(p)));return 1
        @C.CFUNCTYPE(U,P,P,B)
        def ast(p,block,wait):nc.append(('async_status',logical_iface(p),norm(block),wait));return statuses[0]
        @C.CFUNCTYPE(U,P,P,C.POINTER(Q))
        def nsz(p,block,n):nc.append(('network_size',logical_iface(p),norm(block)));n[0]=64;return statuses[1]
        @C.CFUNCTYPE(U,P,P,Q,P,P,C.POINTER(P))
        def nrs(p,block,size,used,buffer,out):
            nc.append(('network_result',logical_iface(p),norm(block),size,norm(used),norm(buffer),norm(C.cast(out,P).value)))
            out[0]=host(META);return statuses[2]
        async_iface=C.create_string_buffer(8);net_iface=C.create_string_buffer(8)
        async_vt=(P*32)();net_vt=(P*32)()
        async_vt[2]=C.cast(rel,P).value;async_vt[3]=C.cast(ast,P).value
        net_vt[2]=C.cast(rel,P).value;net_vt[12]=C.cast(nsz,P).value;net_vt[13]=C.cast(nrs,P).value
        C.memmove(async_iface,struct.pack('<Q',C.addressof(async_vt)),8);C.memmove(net_iface,struct.pack('<Q',C.addressof(net_vt)),8)
        @QUERYFN
        def gquery(cls,iid,out,ctx):
            cls_bytes=C.string_at(cls,16);iid_bytes=C.string_at(iid,16);nc.append(('query',cls_bytes,iid_bytes))
            out[0]=C.addressof(async_iface) if cls_bytes==async_guid else C.addressof(net_iface)
            return 0
        gdk=Gdk(gquery,None)
        @THREE
        def cr(p,e,o,c):nc.append(('crack',norm(p),norm(e)));C.memmove(o+0x14,struct.pack('<I',scheme),4);return statuses[3]
        @THREE
        def opn(p,e,o,c):nc.append(('open',norm(p),norm(e),struct.unpack('<I',C.string_at(o+0x14,4))[0]));return statuses[4]
        @STATUS
        def rg(p,c):nc.append(('register',norm(p)));return statuses[5]
        @OPTIONS
        def ot(p,o,k,c):nc.append(('options',norm(p),norm(o),k));return statuses[6]
        @TWO
        def hd(p,o,c):nc.append(('headers',norm(p),norm(o)));return statuses[7]
        @TWO
        def ct(p,o,c):nc.append(('certificate',norm(p),norm(o)));return error if phase==8 else 0
        svc=Start(C.pointer(rs),C.pointer(bs),C.pointer(gdk),cr,opn,rg,ot,hd,ct,di);fn(*([host(R),host(TASK)] if name=='start_request' else [host(TASK)]),C.byref(svc));raw=bytearray(bytes(native))
        for off in offsets:
            v=struct.unpack_from('<Q',raw,off)[0];struct.pack_into('<Q',raw,off,CB1 if v==ncbptr else norm(v))
        assert nc==oc,(name,i,phase,method,mode,nc,oc)
        assert raw==expected,(name,i,phase,method,mode,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:30])
      print(name,'PASS 512',flush=True)
    print('XCurl original request startup PASS:',total,'cases')
if __name__=='__main__':main()
