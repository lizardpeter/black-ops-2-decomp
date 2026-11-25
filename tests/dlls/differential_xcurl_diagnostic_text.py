#!/usr/bin/env python3
"""Compare A224/A32C diagnostic publication against the pinned Store XCurl.
Only the CRT formatting primitive and the downstream debug callback are
modeled. The original branch, stack-buffer truncation, copying and newline
handling execute as machine instructions.
"""
from differential_xcurl_request import *
EASY=A+0x1000;OUT=A+0x2000;FMT=A+0x3000;INPUT=A+0x4000;CONFIG=A+0x7000;SZ=0x10000
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SZ);lib=C.CDLL(str(library.resolve()))
    rng=random.Random(0xa224);checks=0
    def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
    def q(mu,p):return struct.unpack('<Q',mu.mem_read(p,8))[0]
    for is_error,rva in [(True,0xa224),(False,0xa32c)]:
        fn=getattr(lib,'publish_error_text' if is_error else 'publish_info_text')
        fn.argtypes=(P,C.c_char_p,DEBUG,P) if is_error else (P,C.c_char_p,C.c_bool,DEBUG,P);fn.restype=None
        for i in range(384):
            n=[0,1,2,254,255,256,257,2043,2044,2045,2047,2048,2049,3000,rng.randrange(4001)][i%15]
            raw=bytes(rng.choice(b'abcdefghijklmnop0123456789 %') for _ in range(n))
            newline=i%2==0;fmt=b'%s\n' if newline else b'%s';rendered=raw+(b'\n' if newline else b'')
            verbose=bool(i&2);has_buffer=bool(i&4);has_easy=i%17!=0
            state=bytearray(SZ);state[OUT-A:OUT-A+258]=b'!'*258
            state[EASY-A+0x176]=verbose
            if has_buffer:struct.pack_into('<Q',state,EASY-A+0x160,OUT+1)
            state[FMT-A:FMT-A+len(fmt)+1]=fmt+b'\0'
            state[INPUT-A:INPUT-A+len(raw)+1]=raw+b'\0'
            oc=[];nc=[]
            def format_crt(mu):
                dst,cap,f=[reg(mu,k) for k in ('RDX','R8','R9')]
                assert m.read_cstr(f)==fmt
                va=q(mu,reg(mu,'RSP')+0x30)
                assert q(mu,va)==INPUT
                out=rendered[:cap-1];mu.mem_write(dst,out+b'\0')
                mu.reg_write(X.UC_X86_REG_RAX,len(rendered))
            def suffix(mu):
                dst,cap,f=[reg(mu,k) for k in ('RCX','EDX','R8')]
                data=m.read_cstr(f);assert data in (b'...',b'...\n') and len(data)<cap
                mu.mem_write(dst,data+b'\0');return len(data)
            def debug(mu):
                assert reg(mu,'RCX')==EASY and reg(mu,'EDX')==0
                oc.append(bytes(mu.mem_read(reg(mu,'R8'),reg(mu,'R9'))));return 0
            for name in img.iat.values():
                if name.endswith('!__stdio_common_vsprintf'):m.on_import(name,format_crt)
            def setup(w):m.mu.mem_write(A,bytes(state))
            ret,_,_,_=m.call(img.base+rva,[EASY if has_easy else 0,FMT,INPUT],'void',setup=setup,
                fakes={img.base+0x9210:lambda mu:CONFIG,img.base+0xa128:debug,img.base+0xa438:suffix})
            assert ret is None,(i,ret,m.imports_called)
            want=bytes(m.mu.mem_read(OUT,258))
            native=C.create_string_buffer(bytes(state),SZ);base=C.addressof(native)
            if has_buffer:struct.pack_into('<Q',native,EASY-A+0x160,base+OUT-A+1)
            @DEBUG
            def capture(e,t,p,n,c):
                assert e==base+EASY-A and t==0
                nc.append(C.string_at(p,n))
            args=(base+EASY-A if has_easy else None,rendered)
            if is_error:fn(*args,capture,None)
            else:fn(*args,newline,capture,None)
            got=C.string_at(base+OUT-A,258)
            assert got==want and nc==oc,(is_error,i,n,verbose,has_buffer,has_easy,got[:20],want[:20],[(len(v),v[-12:]) for v in nc],[(len(v),v[-12:]) for v in oc])
            checks+=1
        print(hex(rva),'PASS 384',flush=True)
    print('XCurl original diagnostic publication PASS:',checks,flush=True)
if __name__=='__main__':main()
