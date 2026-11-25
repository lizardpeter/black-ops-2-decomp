#!/usr/bin/env python3
"""Compare 8C50 telemetry batching, record retirement and timestamp.
The original 8970 formatting control flow executes; only the CRT wide
formatter and the final provider sink are modeled. No live ETW is implied.
"""
from differential_xcurl_telemetry import *
PUBLISH=C.CFUNCTYPE(None,P,P)
TICK64=C.CFUNCTYPE(Q,P)
class Flush(C.Structure):
    _fields_=[('lifetime',C.POINTER(Svc)),('tick64',TICK64),('publish',PUBLISH),('context',P)]
CONFIG=A+0x8000
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SZ);lib=C.CDLL(str(library.resolve()))
    fn=lib.xcurl_research_telemetry_flush;fn.argtypes=(P,C.POINTER(Flush));fn.restype=None
    rng=random.Random(0x8c50)
    def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
    def q(mu,p):return struct.unpack('<Q',mu.mem_read(p,8))[0]
    def wide(mu,p):
        data=bytearray()
        for i in range(500):
            v=bytes(mu.mem_read(p+2*i,2))
            if v==b'\0\0':return bytes(data)
            data.extend(v)
        raise AssertionError('unterminated original batch')
    for i in range(320):
        state=bytearray(SZ);ptrs=set();oc=[];nc=[]
        count=[0,1,2,11,12,13,19,20,21,40,41,50,rng.randrange(60)][i%13]
        enabled=i%4!=0;tick=rng.getrandbits(64)
        head=OBJ+0x28;nodes=[NODE+0x40*j for j in range(count)]
        chain=[head,*nodes]
        for j,n in enumerate(chain):
            struct.pack_into('<QQ',state,n-A,chain[(j+1)%len(chain)],chain[j-1]);ptrs.update((n-A,n-A+8))
        for j,n in enumerate(nodes):
            values=[(-1,2,3),(-2147483648,4294967295,18446744073709551615),
                    (rng.randrange(-2147483648,2147483648),rng.getrandbits(32),rng.getrandbits(64))][i%3]
            struct.pack_into('<iIQ',state,n-A+0x10,*values)
        def free(mu):oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
        def format_crt(mu):
            dst,cap,f=[reg(mu,k) for k in ('RDX','R8','R9')]
            assert wide(mu,f).decode('utf-16le')=='%i,%lu,%llu;'
            va=q(mu,reg(mu,'RSP')+0x30)
            a,b,c=struct.unpack('<QQQ',mu.mem_read(va,24))
            a=struct.unpack('<i',struct.pack('<I',a&0xffffffff))[0];b&=0xffffffff
            data=f'{a},{b},{c};'.encode('utf-16le');length=len(data)//2
            mu.mem_write(dst,data[:cap*2]+(b'\0\0' if length<cap else b''))
            mu.reg_write(X.UC_X86_REG_RAX,length if length<=cap else 0xffffffffffffffff)
        def event(mu):
            assert reg(mu,'RDX')==img.base+0x23c91
            ptr=q(mu,q(mu,reg(mu,'RSP')+0x30))
            oc.append(('publish',wide(mu,ptr)));return 0
        for name in img.iat.values():
            if name.endswith('!__stdio_common_vswprintf'):m.on_import(name,format_crt)
            elif name.endswith('!GetTickCount64'):m.on_import(name,lambda mu:mu.reg_write(X.UC_X86_REG_RAX,tick))
        def setup(w):
            m.mu.mem_write(A,bytes(state))
            w.u64(img.base+0x28000,img.base+0x28000);w.u64(img.base+0x28b98,0)
            w.u32(img.base+0x28070,6 if enabled else 0);w.u64(img.base+0x28080,0x400000000000);w.u64(img.base+0x28088,0)
        result,_,_,_=m.call(img.base+0x8c50,[OBJ],'void',setup=setup,
            fakes={img.base+0x9640:free,img.base+0x9210:lambda mu:CONFIG,img.base+0x10c4:event})
        assert result is None,(i,result,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SZ));native=C.create_string_buffer(bytes(state),SZ);base=C.addressof(native)
        def host(v):return base+v-A
        def norm(v):return A+v-base if base<=v<base+SZ else v
        for off in ptrs:struct.pack_into('<Q',native,off,host(struct.unpack_from('<Q',state,off)[0]))
        @FREE
        def release(p,t,c):nc.append(('free',norm(p),t))
        @TICK64
        def tickfn(c):return tick
        @PUBLISH
        def pub(p,c):
            data=bytearray()
            for j in range(500):
                v=C.string_at(p+j*2,2)
                if v==b'\0\0':break
                data.extend(v)
            else:raise AssertionError('unterminated native batch')
            nc.append(('publish',bytes(data)))
        life=Svc(ALLOC(),release,TICK(),None);flush=Flush(C.pointer(life),tickfn,pub if enabled else PUBLISH(),None)
        fn(host(OBJ),C.byref(flush));raw=bytearray(bytes(native))
        for off in ptrs:struct.pack_into('<Q',raw,off,norm(struct.unpack_from('<Q',raw,off)[0]))
        assert nc==oc,(i,count,[(k,len(v) if k=='publish' else v) for k,v,*_ in nc],[(k,len(v) if k=='publish' else v) for k,v,*_ in oc])
        assert raw==expected,(i,count,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:24])
    print('XCurl original telemetry flush PASS: 320 cases',flush=True)
if __name__=='__main__':main()
