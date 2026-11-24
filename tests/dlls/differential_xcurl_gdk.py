#!/usr/bin/env python3
"""Original GDK lazy loader, initialization and query with modeled OS exports."""
from differential_xcurl_request import *
class Runtime(C.Structure):
    _fields_=[('initialized',B),('restricted',B),('module',P),('exports',P*5)]
L=C.CFUNCTYPE(None,P);LOAD=C.CFUNCTYPE(P,C.c_char_p,U,P);RESOLVE=C.CFUNCTYPE(P,P,C.c_char_p,P);INIT=C.CFUNCTYPE(U,P,Q,Q,Q,P);QUERY=C.CFUNCTYPE(U,P,P,P,C.POINTER(P),P);REPORT=C.CFUNCTYPE(None,P,U,C.c_char_p,P);DEBUG_FLAGS=C.CFUNCTYPE(B,P)
class Gdk(C.Structure):
    _fields_=[('lock',L),('unlock',L),('load',LOAD),('resolve',RESOLVE),('initialize',INIT),('query',QUERY),('report',REPORT),('debug_flags',DEBUG_FLAGS),('debug_break',L),('ctx',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);m.mu.mem_map(0x7ffe0000,0x1000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x5148)
    names=[b'QueryApiImpl',b'InitializeApiImpl',b'InitializeApiImplEx',b'XErrorReport',b'UninitializeApiImpl'];funcs=[A+0x1000+i*0x100 for i in range(5)];MODULE=0x12340000;OUT=A+0x100;CLS=A+0x200;IID=A+0x220;IFACE=A+0x300;GLOBAL=img.base+0x289d0
    def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
    def ret(mu,v):mu.reg_write(X.UC_X86_REG_RAX,v)
    for name,rva,types in [('load',0x5364,()),('initialize',0x5148,(Q,)),('query',0x53a4,(P,P,C.POINTER(P)))]:
        fn=getattr(lib,'xcurl_research_gdk_'+name);fn.argtypes=(C.POINTER(Runtime),*types,C.POINTER(Gdk));fn.restype=U
        for i in range(512):
            oc=[];nc=[];init=i%4==0;restricted=i%3==0;module=MODULE if i%2 or init else 0;pre=[f if rng.randrange(3)==0 or init else 0 for f in funcs];available=[f if rng.randrange(5) else 0 for f in funcs];loadok=i%7!=0;flags=rng.getrandbits(64);debug=i%4;status=rng.choice([0,1,0x80004005,0x89240107]);qstatus=rng.choice([0,1,0x80040154,0x80004005]);
            initial=struct.pack('<BB6xQ5Q',int(init),int(restricted),module,*pre)
            def setup(w):
                w.write(GLOBAL,initial);m.mu.mem_write(0x7ffe02d4,bytes([debug]));m.mu.mem_write(OUT,struct.pack('<Q',0x77777777))
                for p in funcs:m.mu.mem_write(p,b'\xc3')
            def lock(mu):assert reg(mu,'RCX')==img.base+0x289c8;oc.append(('lock',))
            def unlock(mu):assert reg(mu,'RCX')==img.base+0x289c8;oc.append(('unlock',))
            def load(mu):assert reg(mu,'RDX')==0;oc.append(('load',m.read_cstr(reg(mu,'RCX')),reg(mu,'R8D')));ret(mu,MODULE if loadok else 0)
            def resolve(mu):n=m.read_cstr(reg(mu,'RDX'));oc.append(('resolve',reg(mu,'RCX'),n));ret(mu,available[names.index(n)])
            def old(mu):oc.append(('initialize',funcs[1],reg(mu,'ECX'),reg(mu,'EDX'),0));return status
            def new(mu):oc.append(('initialize',funcs[2],reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8')));return status
            def query(mu):oc.append(('query',funcs[0],reg(mu,'RCX'),reg(mu,'RDX')));m.mu.mem_write(reg(mu,'R8'),struct.pack('<Q',IFACE));return qstatus
            def report(mu):oc.append(('report',funcs[3],reg(mu,'ECX'),m.read_cstr(reg(mu,'RDX'))));return 0
            def brk(mu):oc.append(('break',))
            imports={'AcquireSRWLockExclusive':lock,'ReleaseSRWLockExclusive':unlock,'LoadLibraryExA':load,'GetProcAddress':resolve,'DebugBreak':brk}
            for n in img.iat.values():
                if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
            args=[] if name=='load' else [flags] if name=='initialize' else [CLS,IID,OUT]
            result,_,writes,_=m.call(img.base+rva,args,'uint',setup=setup,fakes=dict(zip(funcs,[query,old,new,report,lambda mu:0])));assert isinstance(result,int),(name,i,result,m.imports_called)
            expected=bytes(writes.get(GLOBAL+k,bytes([v]))[0] for k,v in enumerate(initial));expected_out=struct.unpack('<Q',m.mu.mem_read(OUT,8))[0]
            native=Runtime.from_buffer_copy(initial);out=P(0x77777777)
            @LOAD
            def nl(n,f,c):nc.append(('load',n,f));return MODULE if loadok else None
            @RESOLVE
            def nr(m,n,c):nc.append(('resolve',m,n));return available[names.index(n)] or None
            @INIT
            def ni(p,a,b,z,c):nc.append(('initialize',p,a,b,z));return status
            @QUERY
            def nq(p,a,b,o,c):nc.append(('query',p,a,b));o[0]=IFACE;return qstatus
            @REPORT
            def rp(p,k,n,c):nc.append(('report',p,k,n))
            svc=Gdk(L(lambda c:nc.append(('lock',))),L(lambda c:nc.append(('unlock',))),nl,nr,ni,nq,rp,DEBUG_FLAGS(lambda c:debug),L(lambda c:nc.append(('break',))),None)
            got=fn(C.byref(native),*(() if name=='load' else (flags,) if name=='initialize' else (CLS,IID,C.byref(out))),C.byref(svc))
            assert (got,nc,bytes(native),out.value)==(result,oc,expected,expected_out),(name,i,got,result,nc,oc,bytes(native).hex(),expected.hex(),out.value,expected_out)
        print(name,'PASS 512',flush=True)
    print('XCurl GDK runtime loader PASS: 1536 original-machine comparisons')
if __name__=='__main__':main()
