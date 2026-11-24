#!/usr/bin/env python3
from differential_xcurl_request import *
CRACK=C.CFUNCTYPE(U,P,P,P);DIAGURL=C.CFUNCTYPE(None,P,U,P,U,P);FORMAT=C.CFUNCTYPE(P,P,P);REPORT=C.CFUNCTYPE(None,U,P,P)
class Url(C.Structure):
    _fields_=[('crack',CRACK),('error',ERROR),('diag',DIAGURL),('format',FORMAT),('report',REPORT),('release',FREE),('ctx',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x14ac0);fn=lib.xcurl_research_crack_url;fn.argtypes=(P,P,P,C.POINTER(Url));fn.restype=U
    URL=A+0x3000;COMP=A+0x4000;MSG=A+0x5000
    for i in range(1024):
        state=bytearray(rng.randbytes(SIZE));oc=[];nc=[];scheme=rng.randrange(6);ok=rng.randrange(4)!=0;err=rng.choice([0,5,12005,0xffffffff,0x80004005]);alloc=rng.randrange(3)!=0
        result=rng.randbytes(0x68);result=result[:0x14]+struct.pack('<I',scheme)+result[0x18:]
        def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
        def crack(mu):
            assert reg(mu,'EDX')==reg(mu,'R8D')==0;out=reg(mu,'R9');oc.append(('crack',reg(mu,'RCX'),out,bytes(mu.mem_read(out,0x68))));mu.mem_write(out,result);mu.reg_write(X.UC_X86_REG_RAX,int(ok))
        def error(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,err)
        def diag(mu):
            site=reg(mu,'RDX')-img.base;oc.append(('diag',reg(mu,'RCX'),site,reg(mu,'R8') if site in (0x20d10,0x20da8) else 0,reg(mu,'R9D') if site==0x20da8 else 0));return 0
        def fmt(mu):assert reg(mu,'RCX')==img.base+0x20d10;oc.append(('format',reg(mu,'RDX')));return MSG if alloc else 0
        def report(mu):oc.append(('report',reg(mu,'ECX'),reg(mu,'RDX')));return 0
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        for n in img.iat.values():
            f={'WinHttpCrackUrl':crack,'GetLastError':error}.get(n.split('!')[-1]);
            if f:m.on_import(n,f)
        def setup(w):m.mu.mem_write(A,bytes(state))
        ret,_,_,_=m.call(img.base+0x14ac0,[URL,E,COMP],'uint',setup=setup,fakes={img.base+0xa224:diag,img.base+0xa32c:diag,img.base+0x1ada4:fmt,img.base+0x54f4:report,img.base+0x9640:free});assert isinstance(ret,int),(i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE));native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def norm(v):return A+v-base if v and base<=v<base+SIZE else v or 0
        def host(v):return base+v-A
        @CRACK
        def cr(p,out,c):nc.append(('crack',norm(p),norm(out),C.string_at(out,0x68)));C.memmove(out,result,0x68);return int(ok)
        @ERROR
        def er(c):nc.append(('error',));return err
        @DIAGURL
        def di(p,k,u,h,c):nc.append(('diag',norm(p),k,norm(u),h))
        @FORMAT
        def fo(p,c):nc.append(('format',norm(p)));return host(MSG) if alloc else None
        @REPORT
        def re(h,p,c):nc.append(('report',h,norm(p)))
        @FREE
        def fr(p,t,c):nc.append(('free',norm(p),t))
        svc=Url(cr,er,di,fo,re,fr,None);got=fn(host(URL),host(E),host(COMP),C.byref(svc))
        assert (got,nc)==(ret,oc),(i,got,ret,nc,oc);assert bytes(native)==expected,i
    print('XCurl URL cracking PASS: 1024 original-machine cases')
if __name__=='__main__':main()
