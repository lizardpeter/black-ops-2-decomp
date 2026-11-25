#!/usr/bin/env python3
"""Original CURLU lifecycle/component differential. Flag-zero individual
getters and null setters only; full parse/format and decode are separate."""
from differential_xcurl_request import *
import itertools
A=0x35000000;SZ=0x20000;SRC=A+0x100;DST=A+0x200;OUT=A+0x300;TEXT=A+0x400;LOWER=A+0x800
DUP=C.CFUNCTYPE(P,P,P)
class Url(C.Structure):
    _fields_=[('allocate',ALLOC),('release',FREE),('duplicate',DUP),('ctx',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SZ);lib=C.CDLL(str(library.resolve()))
    arena=C.create_string_buffer(SZ);base=C.addressof(arena)
    rng=random.Random(0x1d170);total=0
    def native(a):return base+a-A if a else 0
    def norm(a):return A+a-base if a and base<=a<base+SZ else a or 0
    specs=[
      ('url_create',0x1738c,(),P),('url_release_fields',0x1c390,(P,),None),
      ('url_destroy',0x1cfd0,(P,),None),('url_duplicate',0x1d000,(P,),P),
      ('url_move',0x1c610,(P,P),None),('url_clear_part',0x1d740,(P,C.c_int),U),
      ('url_get_part',0x1d170,(P,C.c_int,P),U)]
    for name,rva,types,restype in specs:
      fn=getattr(lib,'xcurl_research_'+name);fn.argtypes=(*types,C.POINTER(Url));fn.restype=restype
      for i in range(240):
        oc=[];nc=[];state=bytearray(SZ);alloc_o=alloc_n=0;next_o=next_n=0x8000
        ptrs=set();objects=[SRC,DST]
        def put(a,v):struct.pack_into('<Q',state,a-A,v);ptrs.add(a-A)
        for o,which in [(SRC,0),(DST,1)]:
            for j in range(12):
                p=A+0x1000+(which*12+j)*0x100
                value=bytes(rng.choice(b'abcABC123-_ ') for _ in range(1+(j+i)%23))+b'\0'
                state[p-A:p-A+len(value)]=value
                put(o+8*j,p if rng.randrange(3) else 0)
            struct.pack_into('<II',state,o-A+0x60,rng.getrandbits(32),rng.getrandbits(32))
        # No scheme-table dependency in the flag-zero port getter.
        if name=='url_get_part' and i%12==6:put(SRC,0)
        put(OUT,0x12345678)
        failure=i%13
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def cstr(mu,p):
            v=bytearray()
            while (x:=mu.mem_read(p+len(v),1)[0]):v.append(x)
            return bytes(v)+b'\0'
        def oa(n,tag):
            nonlocal alloc_o,next_o
            alloc_o+=1;oc.append(('alloc',n,tag))
            if failure==alloc_o:return 0
            p=A+next_o;next_o+=(n+31)&~31
            m.mu.mem_write(p,b'\xa5'*n);return p
        def na(n,tag):
            nonlocal alloc_n,next_n
            alloc_n+=1;nc.append(('alloc',n,tag))
            if failure==alloc_n:return 0
            p=base+next_n;next_n+=(n+31)&~31;C.memset(p,0xa5,n);return p
        def of(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def od(mu):
            value=cstr(mu,reg(mu,'RCX'));p=oa(len(value),5)
            if p:m.mu.mem_write(p,value)
            return p
        @ALLOC
        def allocate(n,tag,c):return na(n,tag)
        @FREE
        def release(p,tag,c):nc.append(('free',norm(p),tag))
        @DUP
        def duplicate(p,c):
            value=C.string_at(p)+b'\0';dst=na(len(value),5)
            if dst:C.memmove(dst,value,len(value))
            return dst
        svc=Url(allocate,release,duplicate,None)
        def setup(w):
            m.mu.mem_write(A,bytes(state));w.u64(img.base+0x28020,img.base+0x17450)
        for k in (0x50,0x58):assert k//8<12
        present=0 if i%17==0 and name in ('url_destroy','url_clear_part','url_get_part') else SRC
        part=i%12;out=0 if i%19==0 else OUT
        if name=='url_create':orig_args=[];new_args=[]
        elif name=='url_move':orig_args=[SRC,DST];new_args=[native(SRC),native(DST)]
        elif name=='url_clear_part':orig_args=[present,part,0,0];new_args=[native(present),part]
        elif name=='url_get_part':
            # Full URL (part zero) is intentionally outside this subset.
            part=11 if part==0 else part
            orig_args=[present,part,out,0];new_args=[native(present),part,native(out)]
        else:orig_args=[present];new_args=[native(present)]
        ret,_,_,_=m.call(img.base+rva,orig_args,'void' if restype is None else 'uint64' if restype==P else 'uint',
          setup=setup,fakes={img.base+0x932c:lambda mu:oa(reg(mu,'RCX'),reg(mu,'EDX')),img.base+0x9640:of,img.base+0x17450:od,img.base+0x1bc4c:lambda mu:0})
        assert ret is None or isinstance(ret,int),(name,i,ret,m.imports_called)
        expected=bytearray(m.mu.mem_read(A,SZ))
        C.memmove(base,bytes(state),SZ)
        for off in ptrs:struct.pack_into('<Q',arena,off,native(struct.unpack_from('<Q',state,off)[0]) if A<=struct.unpack_from('<Q',state,off)[0]<A+SZ else struct.unpack_from('<Q',state,off)[0])
        got=fn(*new_args,C.byref(svc))
        if restype==P:
            got=norm(got)
            if got:objects.append(got)
            ret=ret or 0
        raw=bytearray(bytes(arena))
        # Normalize every pointer in the known original objects and output slot.
        for addr in objects:
            for j in range(12):
                off=addr-A+j*8;struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        struct.pack_into('<Q',raw,OUT-A,norm(struct.unpack_from('<Q',raw,OUT-A)[0]))
        # Duplicate can allocate and free a partially initialized object. Its
        # pointers are still visible in the modeled freed storage.
        if name=='url_duplicate' and not ret and alloc_o>1:
            for j in range(12):
                off=0x8000+j*8;struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert (got,oc)==(ret,nc),(name,i,got,ret,oc,nc)
        assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:24])
        total+=1
      print(name,'PASS 240',flush=True)
    fn=lib.xcurl_research_url_has_scheme;fn.argtypes=(C.c_char_p,P);fn.restype=B
    samples=[b'',b'C:/path',b'x:/',b'https:/',b'HTTPS://a',b'1://',b'://',b'http:foo',b'http://',b'a+b.c-1://',b'a b://',b'a_b://',b'\xff://',b'a'*39+b':/',b'a'*40+b':/',b'a'*41+b':/']
    samples += [bytes(rng.choice(b'AZaz09+-. _:/') for _ in range(i%50))+b':/' for i in range(224)]
    for i,text in enumerate(samples):
        for name in img.iat.values():
            if name.endswith('!tolower'):m.on_import(name,lambda mu:mu.reg_write(X.UC_X86_REG_RAX,ord(chr(mu.reg_read(X.UC_X86_REG_ECX)).lower())))
        def setup(w):m.mu.mem_write(TEXT,text+b'\0');m.mu.mem_write(LOWER,b'\xa5'*64)
        ret,_,_,_=m.call(img.base+0x1bca4,[TEXT,LOWER if i%2 else 0],'uchar',setup=setup)
        out=C.create_string_buffer(b'\xa5'*64,64)
        got=fn(text,out if i%2 else None)
        assert got==ret and bytes(out)==bytes(m.mu.mem_read(LOWER,64)),(text,got,ret,bytes(out),bytes(m.mu.mem_read(LOWER,64)))
        total+=1
    print('url_has_scheme PASS',len(samples),flush=True)
    print('XCurl URL object differential PASS:',total,flush=True)
if __name__=='__main__':main()
