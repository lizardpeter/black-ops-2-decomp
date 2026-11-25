#!/usr/bin/env python3
"""Compare original URL helpers and absolute HTTP(S) parser semantics.
Only the CRT string/format primitives and tagged allocation are modeled.
The actual x64 parser control flow runs in Unicorn; no original parser body
is replaced by a native or third-party parser."""
from differential_xcurl_request import *
import re
A=0x35000000;SIZE=0x40000;OBJ=A+0x100;TEXT=A+0x1000
DUP=C.CFUNCTYPE(P,P,P)
class Svc(C.Structure):
    _fields_=[('allocate',ALLOC),('release',FREE),('duplicate',DUP),('context',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SIZE);lib=C.CDLL(str(library.resolve()))
    arena=C.create_string_buffer(SIZE);base=C.addressof(arena)
    def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
    def cstr(mu,p):
        if not p:return None
        data=bytearray()
        while (v:=mu.mem_read(p+len(data),1)[0]):data.append(v)
        return bytes(data)
    def native(p):return base+p-A
    def sgn(a,b):return (a>b)-(a<b)
    def result(mu,n):mu.reg_write(X.UC_X86_REG_RAX,n&0xffffffffffffffff)
    def strcmp(mu,lower=False,n=None):
        a,b=cstr(mu,reg(mu,'RCX')),cstr(mu,reg(mu,'RDX'))
        if lower:a,b=a.lower(),b.lower()
        if n is not None:a,b=a[:n],b[:n]
        result(mu,sgn(a,b))
    def span(mu,accept):
        a,b=cstr(mu,reg(mu,'RCX')),cstr(mu,reg(mu,'RDX'))
        k=0
        while k<len(a) and ((a[k] in b)==accept):k+=1
        result(mu,k)
    def strtol(mu):
        p=reg(mu,'RCX');v=cstr(mu,p)
        match=re.match(rb'[ \t\r\n\v\f]*[+-]?\d+',v)
        n=int(match.group()) if match else 0
        end=p+(len(match.group()) if match else 0)
        mu.mem_write(reg(mu,'RDX'),struct.pack('<Q',end));result(mu,max(-2147483648,min(2147483647,n)))
    def snprintf(mu):
        fmt=cstr(mu,reg(mu,'R8'));assert fmt==b'%ld',fmt
        v=str(reg(mu,'R9D')).encode()+b'\0';assert len(v)<=reg(mu,'EDX')
        mu.mem_write(reg(mu,'RCX'),v);return len(v)-1
    def scan(mu):
        p=reg(mu,'RCX');fmt=cstr(mu,reg(mu,'RDX'));v=cstr(mu,p)
        if fmt.startswith(b'[%*45'):
            match=re.match(rb'\[[0-9A-Fa-f:.]{1,45}(.)',v)
        else:
            assert fmt==b'%*[^]]%c%n',fmt
            match=re.match(rb'[^\]]+(.)',v)
        if not match:return 0
        mu.mem_write(reg(mu,'R8'),match.group(1));mu.mem_write(reg(mu,'R9'),struct.pack('<I',len(match.group())))
        return 1
    imports={'_stricmp':lambda mu:strcmp(mu,True),'strncmp':lambda mu:strcmp(mu,n=reg(mu,'R8')),
             'strcspn':lambda mu:span(mu,False),'strspn':lambda mu:span(mu,True),'strtol':strtol,
             'tolower':lambda mu:result(mu,ord(chr(reg(mu,'ECX')).lower()))}
    for n in img.iat.values():
        if n.split('!')[-1] in imports:m.on_import(n,imports[n.split('!')[-1]])
    rng=random.Random(0x1c780);checks=0
    paths=[b'',b'.',b'..',b'./',b'../',b'/a/./b',b'/a/../b',b'/a/..',b'/a/.',b'/../x',b'a/../../b',b'/a/../b?x=/../z',b'/a//b/',b'foo?x=y',b'./a/../b']
    paths += [b'/'+b'/'.join(rng.choice([b'a',b'b',b'.',b'..',b'',b'x']) for _ in range(i%9))+rng.choice([b'',b'/',b'?q=/../x']) for i in range(160)]
    hosts=[b'example.test',b'host:',b'host:80',b'host:00080',b'host:0',b'host:65535',b'host:65536',b'host:1x',b'host:+80',b'host: 80',b'[::1]:443',b'[fe80::1%25eth0]:8080',b'[::1]',b'[::1]x',b'bad host',b'',b'[abc]',b'[::1%zone]',b'[::1%25]',b'[::1%]']
    urls=[b'HTTPS://example.test',b'https://example.test:00443/a/./b/../c?q=1#frag',b'http://user:pass@host:80/path',b'http:///host/a',b'http:/host/a',b'http:////host',b'https://host?',b'https://host#',b'https://host/#f',b'https://host:0',b'https://host:65536',b'ftp://host',b'https://bad host',b'https://host/a\nb',b'example.test',b'https://[::1]:443/a',b'https://[fe80::1%25eth0]:8080/a']
    urls += [b'https://host'+rng.choice(paths) for _ in range(100) if True]
    for mode,items,rva,fn_name in [('normalize',paths,0x1ae40,'url_normalize_path'),('port',hosts,0x1bfb4,'url_parse_port'),('host',hosts,0x1c46c,'url_validate_host'),('parse',urls,0x1c744,'url_parse_http')]:
      fn=getattr(lib,'xcurl_research_'+fn_name)
      fn.argtypes=(P,C.POINTER(Svc)) if mode=='normalize' else (P,P,C.POINTER(Svc));fn.restype=P if mode=='normalize' else U
      for value in items:
        oo=nn=0x8000;ol={};nl={}
        def oa(n,tag):
            nonlocal oo
            assert tag==5
            p=A+oo;oo+=(n+31)&~31;assert oo<SIZE;ol[p]=n;m.mu.mem_write(p,b'\xa5'*n);return p
        def od(mu):
            val=cstr(mu,reg(mu,'RCX'))+b'\0';p=oa(len(val),5);m.mu.mem_write(p,val);return p
        def of(mu):
            p=reg(mu,'RCX');assert reg(mu,'EDX')==5 and p in ol,(mode,value,p,ol);del ol[p];return 0
        @ALLOC
        def na(n,tag,c):
            nonlocal nn
            assert tag==5
            p=base+nn;nn+=(n+31)&~31;assert nn<SIZE;nl[p]=n;C.memset(p,0xa5,n);return p
        @FREE
        def nf(p,t,c):
            assert t==5 and p in nl,(mode,value,p,nl);del nl[p]
        @DUP
        def nd(p,c):
            v=C.string_at(p)+b'\0';q=na(len(v),5,None);C.memmove(q,v,len(v));return q
        svc=Svc(na,nf,nd,None)
        def setup(w):
            m.mu.mem_write(A,b'\0'*SIZE);m.mu.mem_write(TEXT,value+b'\0');w.u64(img.base+0x28020,img.base+0x17450)
        args=[TEXT] if mode=='normalize' else [TEXT,OBJ,0] if mode=='parse' else [OBJ,TEXT]
        ret,_,_,_=m.call(img.base+rva,args,'uint64' if mode=='normalize' else 'uint',setup=setup,
          fakes={img.base+0x932c:lambda mu:oa(reg(mu,'RCX'),reg(mu,'EDX')),img.base+0x9640:of,img.base+0x17450:od,img.base+0xa438:snprintf,img.base+0x1bb34:scan})
        assert isinstance(ret,int),(mode,value,ret,m.imports_called)
        C.memset(base,0,SIZE);C.memmove(native(TEXT),value+b'\0',len(value)+1)
        args=[native(TEXT)] if mode=='normalize' else [native(TEXT),native(OBJ)] if mode=='parse' else [native(OBJ),native(TEXT)]
        got=fn(*args,C.byref(svc))
        if mode=='normalize':
            actual=C.string_at(got) if got else None;expected=cstr(m.mu,ret) if ret else None
            assert actual==expected and len(ol)==len(nl),(mode,value,actual,expected,len(ol),len(nl))
        else:
            def fields(read,addr,read_string):
                raw=read(addr,0x68);ptrs=struct.unpack('<12QII',raw)
                return [read_string(p) if p else None for p in ptrs[:12]],ptrs[12]
            original=fields(m.mu.mem_read,OBJ,lambda p:cstr(m.mu,p))
            actual=fields(C.string_at,native(OBJ),C.string_at)
            assert got==ret and actual==original and len(ol)==len(nl),(mode,value,got,ret,actual,original,len(ol),len(nl))
            if mode!='parse':assert C.string_at(native(TEXT))==cstr(m.mu,TEXT),(mode,value)
        checks+=1
      print(mode,'PASS',len(items),flush=True)
    print('XCurl URL parsing differential PASS:',checks,flush=True)
if __name__=='__main__':main()
