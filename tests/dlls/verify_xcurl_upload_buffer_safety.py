#!/usr/bin/env python3
"""Real host allocation regression for the explicitly documented original underflow divergence."""
from differential_xcurl_upload_buffer import *
lib=C.CDLL(str(Path(sys.argv[1]).resolve()));fn=lib.xcurl_research_buffer_upload;fn.argtypes=(P,C.POINTER(P),C.POINTER(Q),C.POINTER(Q),C.POINTER(Buffer));fn.restype=U
native=C.create_string_buffer(0x368);base=C.addressof(native);allocations={};resizes=[];read_count=0;block=64
@ALLOC
def al(n,t,c):
    assert t==16;buf=C.create_string_buffer(n);p=C.addressof(buf);allocations[p]=buf;return p
@REALLOC
def re(p,n,t,c):
    old=allocations.pop(p);buf=C.create_string_buffer(n);C.memmove(buf,old,len(old));p=C.addressof(buf);allocations[p]=buf;resizes.append(n);return p
@FREE
def fr(p,t,c):del allocations[p]
@CB
def cb(p,s,n,u):
    global read_count
    assert s==1 and n==block
    assert any(k<=p and p+n<=k+len(v) for k,v in allocations.items()),'callback out of allocated range'
    read_count+=1
    if read_count==6:return 0
    C.memset(p,read_count,n);return n
struct.pack_into('<I',native,0xb0,block);struct.pack_into('<Q',native,0xf0,0xffffffffffffffff);struct.pack_into('<Q',native,0x120,C.cast(cb,P).value)
no=LOCK(lambda p,c:None);rs=Services(no,no,al,fr,no,no,no,ADJUST(),EVENT(),ERROR(),QUEUE(),DIAG(),DISPATCH(),READ(),RECEIVE(),SEND(),CONTEXT(),CALLBACK(),DEBUG(),no,SUBMIT(),None);bs=Buffer(C.pointer(rs),re,UD(lambda *args:None));out=P();cap=Q();length=Q();rc=fn(base,C.byref(out),C.byref(cap),C.byref(length),C.byref(bs));assert rc==0 and length.value==5*block and read_count==6
assert C.string_at(out,length.value)==b''.join(bytes([i])*block for i in range(1,6));assert resizes==[128,192,256,320,384];fr(out.value,16,None);assert not allocations
print('PASS: 5-block upload and EOF; verified real allocation capacity at every callback; reallocs',resizes)
