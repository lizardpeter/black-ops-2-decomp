#!/usr/bin/env python3
"""Original certificate selector and store lookup, disabled ETW; OS cryptography mocked."""
from differential_xcurl_request import *
OPEN=C.CFUNCTYPE(P,U,P,P);DECODE=C.CFUNCTYPE(U,P,U,P,C.POINTER(U),P);FIND=C.CFUNCTYPE(P,P,P,U,P);SET=C.CFUNCTYPE(None,P,P,U,P);D=C.CFUNCTYPE(None,P,U,U,P,U,P)
class Selector(C.Structure):
    _fields_=[('location',U),('store',P),('length',Q),('thumb',P)]
class Cert(C.Structure):
    _fields_=[('open',OPEN),('decode',DECODE),('find',FIND),('set',SET),('free',LOCK),('close',LOCK),('error',ERROR),('diag',D),('ctx',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()));rng=random.Random(0x10ca0)
    parse=lib.xcurl_research_parse_certificate;parse.argtypes=(P,C.POINTER(Selector));parse.restype=U
    select=lib.xcurl_research_select_certificate;select.argtypes=(P,P,C.POINTER(Cert));select.restype=U
    TEXT=A+0x3000;STORE=A+0x4000;LOC=A+0x80;STR=A+0x90;THUMB=A+0xb0;H=0x12340000;CERT=0x12341000
    names=['CurrentUser','LocalMachine','CurrentService','Services','Users','CurrentUserGroupPolicy','LocalMachineGroupPolicy','LocalMachineEnterprise','invalid','currentuser','']
    def reg(mu,k):return mu.reg_read(getattr(X,'UC_X86_REG_'+k))
    def wide(mu,p):
        data=bytearray()
        while (v:=bytes(mu.mem_read(p+len(data),2)))!=b'\0\0':data.extend(v)
        return data.decode('utf-16le')
    def hostwide(p):
        data=bytearray()
        while (v:=C.string_at(p+len(data),2))!=b'\0\0':data.extend(v)
        return data.decode('utf-16le')
    def cmp(mu):
        a,b,n=[reg(mu,k) for k in ('RCX','RDX','R8')];x=wide(mu,a)[:n];y=wide(mu,b)[:n];mu.reg_write(X.UC_X86_REG_RAX,(int(x>y)-int(x<y))&0xffffffff)
    for name in img.iat.values():
        if name.endswith('!wcsncmp'):m.on_import(name,cmp)
    for i in range(1024):
        state=bytearray(SIZE);oc=[];nc=[];loc=rng.choice(names);loc=loc[:rng.randrange(len(loc)+1)] if i%4==0 else loc;store=rng.choice(['','MY','Root','LongStoreName','x'*32,'é']);thumb=''.join(rng.choice('abcdef0123456789z') for _ in range(rng.choice([0,39,40,40,41])));value=loc+'\\'+store+'\\'+thumb if i%7 else loc if i%14 else loc+'\\'+store;raw=value.encode('utf-16le')+b'\0\0';state[TEXT-A:TEXT-A+len(raw)]=raw
        struct.pack_into('<I',state,LOC-A,0xdeadbeef);struct.pack_into('<QQ',state,STR-A+16,0,7);struct.pack_into('<Q',state,THUMB-A,0x77777777);struct.pack_into('<QQ',state,R-A+0x30,E,0);struct.pack_into('<Q',state,R-A+0x50,HTTP)
        def setup(w):m.mu.mem_write(A,bytes(state))
        def al(mu):assert reg(mu,'EDX')==0;return STORE
        ret,_,_,_=m.call(img.base+0x10ca0,[TEXT,LOC,STR,THUMB],'uint',setup=setup,fakes={img.base+0x932c:al});assert isinstance(ret,int),(i,ret,m.imports_called)
        ol=struct.unpack('<I',m.mu.mem_read(LOC,4))[0];length,cap=struct.unpack('<QQ',m.mu.mem_read(STR+16,16));op=struct.unpack('<Q',m.mu.mem_read(THUMB,8))[0];ostr=bytes(m.mu.mem_read(STR if cap<8 else struct.unpack('<Q',m.mu.mem_read(STR,8))[0],2*length));assert bytes(m.mu.mem_read(TEXT,len(raw)))==raw
        source=C.create_string_buffer(raw);base=C.addressof(source);out=Selector(0xdeadbeef,None,0,0x77777777);got=parse(base,C.byref(out));np=TEXT+out.thumb-base if out.thumb!=0x77777777 else out.thumb;ns=C.string_at(out.store,out.length*2) if out.store else b''
        assert (got,out.location,ns,np)==(ret,ol,ostr,op),(i,value,got,ret,out.location,ol,ns,ostr,np,op)
        # Whole certificate path uses the real original parser and mocked cryptography calls.
        openok=i%3!=0;decodeok=i%5!=0;findok=i%7!=0;err=rng.choice([0,5,12002,0x80004005]);null=i%11==0;hashbytes=rng.randbytes(20)
        def opn(mu):
            assert (reg(mu,'RCX'),reg(mu,'EDX'),reg(mu,'R8'))==(10,0,0);p=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];oc.append(('open',reg(mu,'R9D'),wide(mu,p)));mu.reg_write(X.UC_X86_REG_RAX,H if openok else 0)
        def dec(mu):
            assert reg(mu,'R8D')==4;size=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];assert struct.unpack('<I',mu.mem_read(size,4))[0]==20;oc.append(('decode',wide(mu,reg(mu,'RCX')),reg(mu,'EDX')));mu.mem_write(reg(mu,'R9'),hashbytes);mu.reg_write(X.UC_X86_REG_RAX,int(decodeok))
        def find(mu):
            assert (reg(mu,'EDX'),reg(mu,'R8D'),reg(mu,'R9D'))==(0x10001,0,0x10000);p=struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28,8))[0];n,buf=struct.unpack('<I4xQ',mu.mem_read(p,16));oc.append(('find',reg(mu,'RCX'),bytes(mu.mem_read(buf,n)),n));mu.reg_write(X.UC_X86_REG_RAX,CERT if findok else 0)
        def st(mu):assert reg(mu,'EDX')==47;oc.append(('set',reg(mu,'RCX'),struct.unpack('<Q',mu.mem_read(reg(mu,'R8'),8))[0],reg(mu,'R9D')));mu.reg_write(X.UC_X86_REG_RAX,0)
        def log(mu,k):oc.append((k,reg(mu,'RCX')));mu.reg_write(X.UC_X86_REG_RAX,1)
        def error(mu):oc.append(('error',));mu.reg_write(X.UC_X86_REG_RAX,err)
        def diag(mu):
            site=reg(mu,'RDX')-img.base;a=reg(mu,'R8D') if site!=0x21130 else 0;b=wide(mu,reg(mu,'R8') if site==0x21130 else reg(mu,'R9')) if site!=0x21168 else None;z=struct.unpack('<I',mu.mem_read(reg(mu,'RSP')+0x28,4))[0] if site==0x210f8 else 0;oc.append(('diag',reg(mu,'RCX'),site,a,b,z));return 0
        imports={'CertOpenStore':opn,'CryptStringToBinaryW':dec,'CertFindCertificateInStore':find,'WinHttpSetOption':st,'CertFreeCertificateContext':lambda mu:log(mu,'free'),'CertCloseStore':lambda mu:log(mu,'close'),'GetLastError':error}
        for name in img.iat.values():
            if name.split('!')[-1] in imports:m.on_import(name,imports[name.split('!')[-1]])
        ret,_,_,_=m.call(img.base+0x11478,[R,0 if null else TEXT],'uint',setup=setup,fakes={img.base+0x932c:al,img.base+0x9640:lambda mu:0,img.base+0xa224:diag});assert isinstance(ret,int),(i,value,ret,m.imports_called)
        native=C.create_string_buffer(bytes(state),SIZE);nb=C.addressof(native)
        @OPEN
        def no(f,p,c):nc.append(('open',f,hostwide(p)));return H if openok else None
        @DECODE
        def nd(p,n,b,size,c):nc.append(('decode',hostwide(p),n));assert size[0]==20;C.memmove(b,hashbytes,20);return int(decodeok)
        @FIND
        def nf(h,p,n,c):nc.append(('find',h,C.string_at(p,n),n));return CERT if findok else None
        @SET
        def nst(h,p,n,c):nc.append(('set',h,p,n))
        @ERROR
        def ne(c):nc.append(('error',));return err
        @D
        def dg(e,k,a,p,z,c):nc.append(('diag',e,k,a,hostwide(p) if p else None,z))
        svc=Cert(no,nd,nf,nst,LOCK(lambda p,c:nc.append(('free',p))),LOCK(lambda p,c:nc.append(('close',p))),ne,dg,None);got=select(nb+R-A,None if null else base,C.byref(svc));assert (got,nc)==(ret,oc),(i,value,got,ret,nc,oc)
    print('XCurl certificate selection PASS: 2048 original-machine comparisons')
if __name__=='__main__':main()
