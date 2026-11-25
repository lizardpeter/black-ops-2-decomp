#!/usr/bin/env python3
"""Original telemetry object, owned records, and GUID formatter.

Execute the pinned XCurl instructions. Provider refresh/publish is an
explicit modeled boundary, not a claim of equivalent ETW events.
"""
from differential_xcurl_request import *
A=0x35000000;SZ=0x10000;OBJ=A+0x100;OUT=A+0x200;NODE=A+0x1000
TICK=C.CFUNCTYPE(U,P)
class Svc(C.Structure):
    _fields_=[('allocate',ALLOC),('release',FREE),('tick32',TICK),('context',P)]
def main():
    dll,library=map(Path,sys.argv[1:3]);assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,SZ);lib=C.CDLL(str(library.resolve()))
    arena=C.create_string_buffer(SZ);base=C.addressof(arena);rng=random.Random(0x7a30)
    def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
    def ret(mu,n):mu.reg_write(X.UC_X86_REG_RAX,n)
    def native(p):return base+p-A if p else 0
    def normalize(p):return A+p-base if p and base<=p<base+SZ else p or 0
    specs=[('construct',0x7a30,(P,),P),('create',0x7df4,(P,),U),('unlink',0x7c50,(P,),None),('destroy_records',0x7cf8,(P,),None),('destroy_local',0x80b8,(P,),None)]
    total=0;guid_cases=0
    format_guid=lib.xcurl_research_telemetry_format_guid;format_guid.argtypes=(P,P);format_guid.restype=None
    for name,rva,types,restype in specs:
        fn=getattr(lib,'xcurl_research_telemetry_'+name)
        fn.argtypes=types if name=='unlink' else (*types,C.POINTER(Svc));fn.restype=restype
        for i in range(240):
            oc=[];nc=[];guid_bytes=rng.randbytes(16);tick=rng.getrandbits(32);failure=name=='create' and i%7==0
            state=bytearray(rng.randbytes(SZ));ptrs=set();head=OBJ+0x28;nodes=[NODE+0x100*j for j in range(i%8)]
            def ptr(a,v):struct.pack_into('<Q',state,a-A,v);ptrs.add(a-A)
            def circle(items):
                for j,p in enumerate(items):ptr(p,items[(j+1)%len(items)]);ptr(p+8,items[j-1])
            if name not in ('construct','create'):circle([head,*nodes])
            if name=='unlink' and not nodes:circle([NODE])
            if name=='create':ptr(OUT,0x12345678)
            selected=nodes[i%len(nodes)] if nodes else NODE
            orig_args=[OUT] if name=='create' else [selected] if name=='unlink' else [OBJ]
            new_args=[native(x) for x in orig_args]
            captured=[]
            def capture(mu):
                captured.append(bytes(mu.mem_read(reg(mu,'RSP')+0x30,74)))
            def old_alloc(mu):
                oc.append(('alloc',reg(mu,'RCX'),reg(mu,'EDX')))
                return 0 if failure else OBJ
            def old_free(mu):
                oc.append(('free',reg(mu,'RCX'),reg(mu,'EDX')));return 0
            @ALLOC
            def alloc(n,t,c):
                nc.append(('alloc',n,t));return 0 if failure else native(OBJ)
            @FREE
            def release(p,t,c):nc.append(('free',normalize(p),t))
            @TICK
            def tick32(c):return tick
            svc=Svc(alloc,release,tick32,None)
            def guid(mu):
                m.program_write(reg(mu,'RCX'),guid_bytes);ret(mu,0)
            for n in img.iat.values():
                k=n.split('!')[-1]
                if k=='GetTickCount':m.on_import(n,lambda mu:ret(mu,tick))
                elif k=='GetTickCount64':m.on_import(n,lambda mu:ret(mu,0x100000000+tick))
                elif k=='CoCreateGuid':m.on_import(n,guid)
                elif k in ('EnterCriticalSection','LeaveCriticalSection','EventUnregister'):m.on_import(n,lambda mu:ret(mu,0))
            def setup(w):
                m.mu.mem_write(A,bytes(state))
                # Explicitly disabled providers, without publishing telemetry.
                w.u64(img.base+0x28000,img.base+0x28000)
                w.u64(img.base+0x28050,0);w.u64(img.base+0x28090,0)
                w.u64(img.base+0x28b98,0)
                w.u32(img.base+0x28030,0);w.u32(img.base+0x28070,0)
            result,_,_,_=m.call(img.base+rva,orig_args,'void' if restype is None else 'uint64' if restype==P else 'uint',
                setup=setup,fakes={img.base+0x932c:old_alloc,img.base+0x9640:old_free,
                    img.base+0x8fd0:lambda mu:0,img.base+0x8bc0:lambda mu:0,img.base+0x8c50:lambda mu:0},
                at={img.base+0x7b87:capture})
            assert result is None or isinstance(result,int),(name,i,result,m.imports_called)
            expected=bytes(m.mu.mem_read(A,SZ));C.memmove(base,bytes(state),SZ)
            for off in ptrs:
                p=struct.unpack_from('<Q',state,off)[0]
                struct.pack_into('<Q',arena,off,native(p) if A<=p<A+SZ else p)
            got=fn(*new_args) if name=='unlink' else fn(*new_args,C.byref(svc))
            if restype==P:got=normalize(got)
            raw=bytearray(bytes(arena))
            if name in ('construct','create') and not failure:ptrs.update((OBJ-A+0x28,OBJ-A+0x30))
            for off in ptrs:struct.pack_into('<Q',raw,off,normalize(struct.unpack_from('<Q',raw,off)[0]))
            assert (got,nc)==(result,oc),(name,i,got,result,nc,oc)
            assert raw==expected,(name,i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:24])
            if captured:
                assert len(captured)==1
                g=C.create_string_buffer(guid_bytes);out=C.create_string_buffer(74);format_guid(g,out)
                assert bytes(out)==captured[0],(guid_bytes,bytes(out),captured)
                guid_cases+=1
            total+=1
        print(name,'PASS 240',flush=True)
    assert guid_cases==240+240-35,guid_cases
    print('GUID format PASS',guid_cases,flush=True)
    print('XCurl telemetry lifetime differential PASS:',total,flush=True)
if __name__=='__main__':main()
