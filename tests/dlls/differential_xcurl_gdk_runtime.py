#!/usr/bin/env python3
"""Original XCurl GDK LoadLibraryExA/GetProcAddress/init/QueryApiImpl differential."""
from differential_xcurl_upload_buffer import *
class Runtime(C.Structure):
    _fields_=[('initialized',B),('restricted',B),('reserved',C.c_uint8*6),('module',P),('exports',P*5)]
LOCK=C.CFUNCTYPE(None,P)
LOAD=C.CFUNCTYPE(P,C.c_char_p,U,P)
RESOLVE=C.CFUNCTYPE(P,P,C.c_char_p,P)
INIT=C.CFUNCTYPE(U,P,Q,Q,Q,P)
QUERY=C.CFUNCTYPE(U,P,P,P,C.POINTER(P),P)
REPORT=C.CFUNCTYPE(None,P,U,C.c_char_p,P)
DEBUG_FLAGS=C.CFUNCTYPE(B,P)
class Services(C.Structure):
    _fields_=[('lock',LOCK),('unlock',LOCK),('load',LOAD),('resolve',RESOLVE),('initialize',INIT),('query',QUERY),('report',REPORT),('debug_flags',DEBUG_FLAGS),('debug_break',LOCK),('context',P)]
def cstr(value):
    if isinstance(value,str):return value
    if isinstance(value,bytes):return value.decode('ascii')
    raw=C.cast(value,C.c_char_p).value
    return raw.decode('ascii') if raw else ''
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);m.mu.mem_map(0x7ffe0000,0x1000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x5148)
    names=[b'QueryApiImpl',b'InitializeApiImpl',b'InitializeApiImplEx',b'XErrorReport',b'UninitializeApiImpl']
    funcs=[A+0x1000+i*0x100 for i in range(5)];MODULE=0x12340000;OUT=A+0x100;CLS=A+0x200;IID=A+0x220;IFACE=A+0x300
    def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
    for name,rva,argtypes in [('load',0x5364,()),('initialize',0x5148,(Q,)),('query',0x53a4,(P,P,C.POINTER(P)))]:
      fn=getattr(lib,'xcurl_research_gdk_'+name);fn.argtypes=(C.POINTER(Runtime),*argtypes,C.POINTER(Services));fn.restype=U
      for i in range(512):
        oc=[];nc=[];init=i%4==0;restricted=i%3==0;module=MODULE if i%2 or init else 0
        pre=[f if rng.randrange(3)==0 or init else 0 for f in funcs]
        available=[f if rng.randrange(5) else 0 for f in funcs]
        loadok=i%7!=0;flags=rng.getrandbits(64);debug=i%4
        status=rng.choice([0,1,0x80004005,0x89240107]);qstatus=rng.choice([0,1,0x80040154,0x80004005])
        initial=struct.pack('<BB6xQ5Q',int(init),int(restricted),module,*pre)
        def setup(w):
            m.mu.mem_write(img.base+0x289d0,initial);m.mu.mem_write(0x7ffe02d4,bytes([debug]))
            m.mu.mem_write(A,bytes(0x6000));m.mu.mem_write(OUT,struct.pack('<Q',0x77777777))
            for p in funcs:m.mu.mem_write(p,b'\xc3')
        def lock(mu):oc.append(('lock',))
        def unlock(mu):oc.append(('unlock',))
        def load(mu):
            oc.append(('load',m.read_cstr(reg(mu,'RCX')).decode('ascii'),reg(mu,'EDX'),reg(mu,'R8D')))
            mu.reg_write(X.UC_X86_REG_RAX,MODULE if loadok else 0)
        def resolve(mu):
            raw=m.read_cstr(reg(mu,'RDX'));n=raw.decode('ascii');oc.append(('resolve',reg(mu,'RCX'),n));mu.reg_write(X.UC_X86_REG_RAX,available[names.index(raw)])
        def old(mu):
            oc.append(('initialize',funcs[1],reg(mu,'ECX'),reg(mu,'EDX'),0));mu.reg_write(X.UC_X86_REG_RAX,status)
        def new(mu):
            oc.append(('initialize',funcs[2],reg(mu,'RCX'),reg(mu,'RDX'),reg(mu,'R8')));mu.reg_write(X.UC_X86_REG_RAX,status)
        def query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16));oc.append(('query',funcs[0],cls,iid))
            mu.mem_write(reg(mu,'R8'),struct.pack('<Q',IFACE));mu.reg_write(X.UC_X86_REG_RAX,qstatus)
        def report(mu):
            oc.append(('report',funcs[3],reg(mu,'ECX'),m.read_cstr(reg(mu,'RDX')).decode('ascii')));mu.reg_write(X.UC_X86_REG_RAX,0)
        def brk(mu):oc.append(('break',))
        imports={'AcquireSRWLockExclusive':lock,'ReleaseSRWLockExclusive':unlock,'LoadLibraryExA':load,'GetProcAddress':resolve,'DebugBreak':brk}
        for n in img.iat.values():
            if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
        fakes=dict(zip(funcs,[query,old,new,report,lambda mu:0]))
        args=[] if name=='load' else [flags] if name=='initialize' else [CLS,IID,OUT]
        ret,_,writes,_=m.call(img.base+rva,args,'uint',setup=setup,fakes=fakes)
        assert isinstance(ret,int),(name,i,ret,m.imports_called)
        expected_state=bytes(writes.get(img.base+0x289d0+k,bytes([v]))[0] for k,v in enumerate(initial))
        expected_out=struct.unpack('<Q',m.mu.mem_read(OUT,8))[0]  # host callback writes bypass Unicorn's instruction journal

        native=Runtime.from_buffer_copy(initial);out=P(0x77777777)
        @LOCK
        def nl(c):nc.append(('lock',))
        @LOCK
        def nu(c):nc.append(('unlock',))
        @LOAD
        def nload(n,f,c):nc.append(('load',cstr(n),0,f));return MODULE if loadok else None
        @RESOLVE
        def nr(mod,n,c):
            name=cstr(n);nc.append(('resolve',mod,name));return available[names.index(name.encode('ascii'))] or None
        @INIT
        def ni(p,a,b,z,c):
            if a>0xffffffff: nc.append(('initialize',p,a,b,z))
            else: nc.append(('initialize',p,a,b,0))
            return status
        @QUERY
        def nq(p,a,b,o,c):
            nc.append(('query',p,C.string_at(a,16),C.string_at(b,16)));o[0]=IFACE;return qstatus
        @REPORT
        def rp(p,k,n,c):nc.append(('report',p,k,cstr(n)))
        @DEBUG_FLAGS
        def df(c):return debug
        @LOCK
        def db(c):nc.append(('break',))
        svc=Services(nl,nu,nload,nr,ni,nq,rp,df,db,None)
        if name=='load':got=fn(C.byref(native),C.byref(svc))
        elif name=='initialize':got=fn(C.byref(native),flags,C.byref(svc))
        else:
            cls_buffer=C.create_string_buffer(16);iid_buffer=C.create_string_buffer(16)
            got=fn(C.byref(native),C.addressof(cls_buffer),C.addressof(iid_buffer),C.byref(out),C.byref(svc))
        raw=bytes(native)
        assert (got,nc,raw,out.value)==(ret,oc,expected_state,expected_out),(name,i,got,ret,nc,oc,raw.hex(),expected_state.hex(),out.value,expected_out)
      print(name,'PASS 512',flush=True)
    print('XCurl GDK runtime PASS: 1536 original-machine comparisons')
if __name__=='__main__':main()
