#!/usr/bin/env python3
"""Original response delivery chunking, callback results and accounting."""
from differential_xcurl_request import *
WRITE=C.CFUNCTYPE(Q,P,P,Q,P,P);DDIAG=C.CFUNCTYPE(None,P,U,Q,U,P)
class Delivery(C.Structure):
    _fields_=[('request',C.POINTER(Services)),('write',WRITE),('diag',DDIAG)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0xab3c)
    fn=lib.xcurl_research_deliver_body;fn.argtypes=(P,P,U,C.POINTER(Delivery));fn.restype=U
    for i in range(512):
        oc=[];nc=[];state=bytearray(SIZE);cb=0x13570000;ud=0x13580000;limit=[1,7,16,64,0x2000,0xffffffff][i%6];length=[0,1,8,64,255,512][(i//6)%6];mode=(i//36)%5;debug=i%2;cut=(i//180)%3
        struct.pack_into('<I',state,E-A+0xac,limit);struct.pack_into('<QQ',state,E-A+0x108,ud,cb);state[E-A+0x176]=debug;struct.pack_into('<I',state,E-A+0x33c,rng.getrandbits(32))
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def log(mu,k):oc.append((k,reg(mu,'RCX')))
        def wr_value(trace,n):
            if sum(t[0]=='write' for t in trace)<=cut:return n
            return [n,0x10000001,0,0xffffffffffffffff,n+1][mode]
        def write(mu):
            n=reg(mu,'R8');oc.append(('write',reg(mu,'RCX'),reg(mu,'RDX'),n,reg(mu,'R9')));return wr_value(oc,n)
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,0 if site==0x20958 else reg(mu,'R8'),0 if site==0x20958 else reg(mu,'R9D')));return 0
        def dbg(mu):oc.append(('debug',reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8'),reg(mu,'R9')));return 0
        for n in img.iat.values():
            k=n.split('!')[-1]
            if k in ('EnterCriticalSection','LeaveCriticalSection'):m.on_import(n,lambda mu,k=k:log(mu,'enter' if k=='EnterCriticalSection' else 'leave'))
        def setup(w):m.mu.mem_write(A,bytes(state))
        result,_,_,_=m.call(img.base+0xab3c,[E,BUF,length],'uint',setup=setup,fakes={img.base+0xb9b4:write,img.base+0xa224:diag,img.base+0xa128:dbg})
        assert isinstance(result,int),(i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def lock(k):return LOCK(lambda p,c:nc.append((k,norm(p))))
        @WRITE
        def nw(f,b,n,u,c):nc.append(('write',norm(f),norm(b),n,norm(u)));return wr_value(nc,n)
        @DDIAG
        def nd(e,s,v,n,c):nc.append(('diag',norm(e),s,v,n))
        @DEBUG
        def nb(e,t,b,n,c):nc.append(('debug',norm(e),t,norm(b),n))
        rs=Services(lock('enter'),lock('leave'),ALLOC(),FREE(),LOCK(),LOCK(),LOCK(),ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),nb,LOCK(),SUBMIT(),None)
        ds=Delivery(C.pointer(rs),nw,nd);got=fn(base+E-A,base+BUF-A,length,C.byref(ds))
        assert (got,nc)==(result,oc),(i,got,result,nc,oc)
        assert bytes(native)==expected,(i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(bytes(native),expected)) if a!=b][:30])
    print('XCurl callback delivery PASS 512 cases')
if __name__=='__main__':main()
