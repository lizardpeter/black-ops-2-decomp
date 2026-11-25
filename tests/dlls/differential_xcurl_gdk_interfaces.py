#!/usr/bin/env python3
"""Differentially exercise every XCurl GDK/COM-style wrapper against original machine code."""
from differential_xcurl_upload_buffer import *

QUERYFN=C.CFUNCTYPE(U,P,P,C.POINTER(P),P)
class Gdk(C.Structure):
    _fields_=[('query',QUERYFN),('context',P)]

ASYNC_GUID_RVA=0x20120
NET_CLASS_RVA=0x20130
NET_IID_RVA=0x20140
ASYNC_IFACE=A+0x2000
NET_IFACE=A+0x2100
ASYNC_VT=A+0x2200
NET_VT=A+0x2300
QUERY=A+0x3000
METHOD=A+0x3100
ASSERT_METHOD=A+0x3200
RELEASE=A+0x3300
TASK=A+0x1000
URL=A+0x1100
QUEUE=A+0x1200
CONTEXT=A+0x1300
CALLBACK=A+0x1400
TOKEN_OUT=A+0x1500
INFO=A+0x1600
OUT=A+0x1700
OUT2=A+0x1710
BUFFER=A+0x1800

CASES=[
 dict(name='error_report',rva=0x54f4,fn='xcurl_research_gdk_error_report',
      args=lambda r:[r.getrandbits(32),URL],call_kinds=['u','p'],machine_ret='void',native_ret=None,iface='net',slot=3,
      method_kinds=['u','p'],reg_args=2,stack_args=0,method_ret=None,assert_before=True),
 dict(name='async_cancel',rva=0x5424,fn='xcurl_research_gdk_async_cancel',
      args=lambda r:[TASK],call_kinds=['p'],machine_ret='void',native_ret=None,iface='async',slot=5,
      method_kinds=['p'],reg_args=1,stack_args=0,method_ret=None,assert_before=False),
 dict(name='async_get_status',rva=0x547c,fn='xcurl_research_gdk_async_get_status',
      args=lambda r:[TASK,r.randrange(2)],call_kinds=['p','b'],machine_ret='uint',native_ret=U,iface='async',slot=3,
      method_kinds=['p','b'],reg_args=2,stack_args=0,method_ret=U,assert_before=False),
 dict(name='network_get_connectivity_hint',rva=0x5600,fn='xcurl_research_gdk_network_get_connectivity_hint',
      args=lambda r:[OUT],call_kinds=['p'],machine_ret='uint',native_ret=U,iface='net',slot=15,
      method_kinds=['p'],reg_args=1,stack_args=0,method_ret=U,assert_before=False,
      output=('bytes',0,bytes.fromhex('1122334455667788'))),
 dict(name='network_security_async',rva=0x566c,fn='xcurl_research_gdk_network_security_async',
      args=lambda r:[URL,TASK],call_kinds=['p','p'],machine_ret='uint',native_ret=U,iface='net',slot=11,
      method_kinds=['p','p'],reg_args=2,stack_args=0,method_ret=U,assert_before=False),
 dict(name='network_security_result',rva=0x56e8,fn='xcurl_research_gdk_network_security_result',
      args=lambda r:[TASK,r.choice([32,64,128]),0,BUFFER,OUT2],call_kinds=['p','q','p','p','p'],
      machine_ret='uint',native_ret=U,iface='net',slot=13,method_kinds=['p','q','p','p','pp'],
      reg_args=3,stack_args=2,method_ret=U,assert_before=False,
      output=('ptr',4,A+0x1900)),
 dict(name='network_security_result_size',rva=0x5790,fn='xcurl_research_gdk_network_security_result_size',
      args=lambda r:[TASK,OUT],call_kinds=['p','p'],machine_ret='uint',native_ret=U,iface='net',slot=12,
      method_kinds=['p','p'],reg_args=2,stack_args=0,method_ret=U,assert_before=False,
      output=('qword',1,0x123456789abcdef0)),
 dict(name='network_register_connectivity',rva=0x580c,fn='xcurl_research_gdk_network_register_connectivity',
      args=lambda r:[QUEUE,CONTEXT,CALLBACK,TOKEN_OUT],call_kinds=['p','p','p','p'],
      machine_ret='uint',native_ret=U,iface='net',slot=16,method_kinds=['p','p','p','p'],
      reg_args=3,stack_args=1,method_ret=U,assert_before=False,
      output=('qword',3,0xfeed1234)),
 dict(name='network_unregister_connectivity',rva=0x58ac,fn='xcurl_research_gdk_network_unregister_connectivity',
      args=lambda r:[r.getrandbits(64),r.randrange(2)],call_kinds=['q','b'],
      machine_ret='uchar',native_ret=B,iface='net',slot=17,method_kinds=['q','b'],
      reg_args=2,stack_args=0,method_ret=B,assert_before=True),
 dict(name='network_verify_certificate',rva=0x5934,fn='xcurl_research_gdk_network_verify_certificate',
      args=lambda r:[TASK,INFO],call_kinds=['p','p'],machine_ret='uint',native_ret=U,iface='net',slot=14,
      method_kinds=['p','p'],reg_args=2,stack_args=0,method_ret=U,assert_before=True),
 dict(name='task_queue_close',rva=0x5ab4,fn='xcurl_research_gdk_task_queue_close',
      args=lambda r:[QUEUE],call_kinds=['p'],machine_ret='void',native_ret=None,iface='async',slot=17,
      method_kinds=['p'],reg_args=1,stack_args=0,method_ret=None,assert_before=False),
 dict(name='task_queue_create',rva=0x5b10,fn='xcurl_research_gdk_task_queue_create',
      args=lambda r:[r.randrange(4),r.randrange(4),OUT],call_kinds=['u','u','p'],
      machine_ret='uint',native_ret=U,iface='async',slot=12,method_kinds=['u','u','pp'],
      reg_args=3,stack_args=0,method_ret=U,assert_before=True,
      output=('ptr',2,A+0x1a00)),
 dict(name='task_queue_dispatch',rva=0x5b98,fn='xcurl_research_gdk_task_queue_dispatch',
      args=lambda r:[QUEUE,r.randrange(3),r.randrange(1000)],call_kinds=['p','u','u'],
      machine_ret='uchar',native_ret=B,iface='async',slot=16,method_kinds=['p','u','u'],
      reg_args=3,stack_args=0,method_ret=B,assert_before=False),
 dict(name='task_queue_submit',rva=0x5c20,fn='xcurl_research_gdk_task_queue_submit',
      args=lambda r:[QUEUE,r.randrange(3),CONTEXT,CALLBACK],call_kinds=['p','u','p','p'],
      machine_ret='uint',native_ret=U,iface='async',slot=18,method_kinds=['p','u','p','p'],
      reg_args=3,stack_args=1,method_ret=U,assert_before=False),
 dict(name='task_queue_terminate',rva=0x5cbc,fn='xcurl_research_gdk_task_queue_terminate',
      args=lambda r:[QUEUE,r.randrange(2),CONTEXT,CALLBACK],call_kinds=['p','b','p','p'],
      machine_ret='uint',native_ret=U,iface='async',slot=22,method_kinds=['p','b','p','p'],
      reg_args=3,stack_args=1,method_ret=U,assert_before=True),
 dict(name='thread_assert_not_time_sensitive',rva=0x5d5c,fn='xcurl_research_gdk_thread_assert_not_time_sensitive',
      args=lambda r:[],call_kinds=[],machine_ret='void',native_ret=None,iface='async',slot=29,
      method_kinds=[],reg_args=0,stack_args=0,method_ret=None,assert_before=False,is_assert=True),
]

TYPE_MAP={'p':P,'pp':C.POINTER(P),'q':Q,'u':U,'b':B}
def main():
    dll,library=map(Path,sys.argv[1:3])
    assert hashlib.sha256(dll.read_bytes()).hexdigest()==SHA256
    img=Image(dll);m=Machine(img);m.mu.mem_map(A,0x10000);lib=C.CDLL(str(library.resolve()))
    async_guid=bytes(m.mu.mem_read(img.base+ASYNC_GUID_RVA,16))
    net_class=bytes(m.mu.mem_read(img.base+NET_CLASS_RVA,16))
    net_iid=bytes(m.mu.mem_read(img.base+NET_IID_RVA,16))
    rng=random.Random(0x5d5c);total=0
    for case in CASES:
      fn=getattr(lib,case['fn'])
      fn.argtypes=[TYPE_MAP[k] for k in case['call_kinds']]+[C.POINTER(Gdk)]
      fn.restype=case['native_ret']
      for i in range(192):
        total+=1;oc=[];nc=[];state=bytearray(SIZE);offsets=set()
        hr=rng.choice([0,1,0x80004005,0x89240107]);boolean=rng.randrange(2)
        output=case.get('output');query_results=[0,1,0x80004005,0x89240101]
        def query_status(n):return query_results[(i//24+n*3)%4]
        def q(a,v):struct.pack_into('<Q',state,a-A,v&0xffffffffffffffff);offsets.add(a-A)
        def put32(a,v):struct.pack_into('<I',state,a-A,v&0xffffffff)
        q(ASYNC_IFACE,ASYNC_VT);q(NET_IFACE,NET_VT)
        q(ASYNC_VT+2*8,RELEASE);q(NET_VT+2*8,RELEASE)
        q(ASYNC_VT+29*8,ASSERT_METHOD);q(NET_VT+29*8,ASSERT_METHOD)
        if case.get('is_assert'):q(ASYNC_VT+29*8,METHOD)
        (q(ASYNC_VT+case['slot']*8,METHOD) if case['iface']=='async' else q(NET_VT+case['slot']*8,METHOD))
        q(OUT,0);q(OUT2,0);q(TOKEN_OUT,0);q(TASK,0);q(QUEUE,0);q(CONTEXT,0);q(CALLBACK,0);q(INFO,0);q(BUFFER,0)
        url='https://example.invalid/security'.encode('utf-16le')+b'\\0\\0'
        state[URL-A:URL-A+len(url)]=url
        args=case['args'](rng)
        for k,v in zip(case['call_kinds'],args):
            if k=='p' and isinstance(v,int) and A<=v<A+SIZE:
                offsets.add(v-A if v-A in (OUT-A,OUT2-A,TOKEN_OUT-A) else v-A)
        def reg(mu,n):return mu.reg_read(getattr(X,'UC_X86_REG_'+n))
        def write_orig(mu,vals):
            if not output:return
            kind,index,value=output;ptr=vals[index+1]
            if kind=='bytes':mu.mem_write(ptr,value)
            elif kind=='qword':mu.mem_write(ptr,struct.pack('<Q',value))
            elif kind=='ptr':mu.mem_write(ptr,struct.pack('<Q',value))
        def query(mu):
            cls=bytes(mu.mem_read(reg(mu,'RCX'),16));iid=bytes(mu.mem_read(reg(mu,'RDX'),16))
            status=query_status(sum(t[0]=='query' for t in oc));oc.append(('query',cls,iid));iface=ASYNC_IFACE if cls==async_guid else NET_IFACE
            if not status&0x80000000:mu.mem_write(reg(mu,'R8'),struct.pack('<Q',iface))
            mu.reg_write(X.UC_X86_REG_RAX,status)
        def release(mu):oc.append(('release',reg(mu,'RCX')));mu.reg_write(X.UC_X86_REG_RAX,1)
        def assert_method(mu):oc.append(('method', (reg(mu,'RCX'),)))
        def method(mu):
            names=['RDX','R8','R9'];vals=[reg(mu,'RCX')]
            for name,kind in zip(names[:case['reg_args']],case['method_kinds'][:case['reg_args']]):
                value=reg(mu,name)
                if kind=='u':value&=0xffffffff
                elif kind=='b':value&=0xff
                vals.append(value)
            for j in range(case['stack_args']):
                vals.append(struct.unpack('<Q',mu.mem_read(reg(mu,'RSP')+0x28+8*j,8))[0])
            oc.append(('method',tuple(vals)));write_orig(mu,vals)
            if case['method_ret'] is U:mu.reg_write(X.UC_X86_REG_RAX,hr)
            elif case['method_ret'] is B:mu.reg_write(X.UC_X86_REG_RAX,boolean)
        def setup(w):
            m.mu.mem_write(A,bytes(state))
            m.mu.mem_write(img.base+0x289d0,bytes([1,0]))
            m.mu.mem_write(img.base+0x289d8,struct.pack('<Q',0x12345678))
            m.mu.mem_write(img.base+0x289e0,struct.pack('<Q',QUERY))
        fakes={QUERY:query,RELEASE:release,ASSERT_METHOD:assert_method,METHOD:method}
        ret,_,_,_=m.call(img.base+case['rva'],args,case['machine_ret'],setup=setup,fakes=fakes)
        assert not isinstance(ret,str),(case['name'],i,ret,m.imports_called)
        expected=bytes(m.mu.mem_read(A,SIZE))
        native=C.create_string_buffer(bytes(state),SIZE);base=C.addressof(native)
        def host(v):return base+v-A if v and A<=v<A+SIZE else v
        def norm(v):
            if not v:return 0
            if base<=v<base+SIZE:return A+v-base
            if v==C.addressof(async_iface):return ASYNC_IFACE
            if v==C.addressof(net_iface):return NET_IFACE
            return v
        def logical_iface(v):
            if v==C.addressof(async_iface):return ASYNC_IFACE
            if v==C.addressof(net_iface):return NET_IFACE
            return norm(v)
        @C.CFUNCTYPE(U,P)
        def rel(p):nc.append(('release',logical_iface(p)));return 1
        @C.CFUNCTYPE(None,P)
        def assert_cb(p):nc.append(('method',(logical_iface(p),)))
        method_types=[P]+[TYPE_MAP[k] for k in case['method_kinds']]
        def native_method(*values):
            vals=[logical_iface(values[0])]
            for kind,value in zip(case['method_kinds'],values[1:]):
                if kind=='p':vals.append(norm(value))
                elif kind=='pp':vals.append(norm(C.cast(value,P).value))
                else:vals.append(int(value))
            nc.append(('method',tuple(vals)))
            if output:
                kind,index,value=output;ptr=values[index+1]
                if kind=='bytes':C.memmove(ptr,value,len(value))
                elif kind=='qword':C.cast(ptr,C.POINTER(Q))[0]=value
                elif kind=='ptr':C.cast(ptr,C.POINTER(P))[0]=host(value)
            if case['method_ret'] is U:return hr
            if case['method_ret'] is B:return boolean
            return None
        method_cb=C.CFUNCTYPE(case['method_ret'],*method_types)(native_method)
        async_iface=C.create_string_buffer(8);net_iface=C.create_string_buffer(8)
        async_vt=(P*32)();net_vt=(P*32)()
        async_vt[2]=C.cast(rel,P).value;net_vt[2]=C.cast(rel,P).value
        async_vt[29]=C.cast(method_cb if case.get('is_assert') else assert_cb,P).value
        net_vt[29]=C.cast(assert_cb,P).value
        (async_vt if case['iface']=='async' else net_vt)[case['slot']]=C.cast(method_cb,P).value
        C.memmove(async_iface,struct.pack('<Q',C.addressof(async_vt)),8)
        C.memmove(net_iface,struct.pack('<Q',C.addressof(net_vt)),8)
        @QUERYFN
        def gquery(cls,iid,out,ctx):
            cb=C.string_at(cls,16);ib=C.string_at(iid,16);status=query_status(sum(t[0]=='query' for t in nc));nc.append(('query',cb,ib))
            if not status&0x80000000:out[0]=C.addressof(async_iface) if cb==async_guid else C.addressof(net_iface)
            return status
        gdk=Gdk(gquery,None)
        native_args=[host(v) if k=='p' else v for k,v in zip(case['call_kinds'],args)]
        got=fn(*native_args,C.byref(gdk))
        raw=bytearray(bytes(native))
        for off in offsets:
            v=struct.unpack_from('<Q',raw,off)[0];struct.pack_into('<Q',raw,off,norm(v))
        assert (got,nc)==(ret,oc),(case['name'],i,got,ret,nc,oc)
        assert raw==expected,(case['name'],i,[(hex(j),a,b) for j,(a,b) in enumerate(zip(raw,expected)) if a!=b][:20])
      print(case['name'],'PASS 192',flush=True)
    print('XCurl GDK interface wrappers PASS:',total,'original-machine comparisons')

if __name__=='__main__':main()
