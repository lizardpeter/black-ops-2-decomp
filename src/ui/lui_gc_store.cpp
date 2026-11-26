// Original Store HKS incremental collection and write barrier bodies.
#define T6_SOURCE "ui/lui_hks.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_lui_gc{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
struct Value{U tag,aux;Q payload;};
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern U GC_ValueBytes,GC_ObjectBytes,GC_ScratchBytes;
U GC_Advance(B*,void*);
bool GC_Check(B*);
void GC_OutOfMemory(void*);
}
using Alloc=void*(*)(void*,void*,Q,Q);
using Interrupt=int(*)(void*);
static void* Resize(B*g,void*p,Q old,Q size){return R<Alloc>(g,0)(R<void*>(g,8),p,old,size);}
static void Account(B*g,Q n,U&counter){counter+=U(n);Q used=R<Q>(g,24)+n;W(g,24,used);if(used>R<Q>(g,32))W(g,32,used);}
static void Release(B*g,void*p,Q size,U&counter){counter-=U(size);W(g,24,R<Q>(g,24)-size);Resize(g,p,size,0);}
static B* Expand(B*gc,unsigned off,Q cap,Q initial,Q stride,U&counter){
    Q newcap=cap?cap*2:initial;
    void*old=R<void*>(gc,off);B*g;void*mem;
    do{
        g=R<B*>(gc,0x58);mem=Resize(g,nullptr,0,newcap*stride);
        if(!mem){W<void*>(gc,off,nullptr);GC_OutOfMemory(R<void*>(gc,0x48));}
    }while(!mem);
    Account(g,newcap*stride,counter);W(gc,off,mem);
    if(old){std::memcpy(mem,old,cap*stride);Release(R<B*>(R<B*>(gc,0x48),0x10),old,cap*stride,counter);}
    return static_cast<B*>(mem);
}
// 140692140
void BarrierValue(B*gc,const B*parent,const Value*v){
    U type=v->tag&15;
    if(type-4>=7||!(R<U>(parent,0)&1))return;
    B*object=reinterpret_cast<B*>(v->payload);
    if(type==4&&static_cast<long long>(R<Q>(object,8))<0)return;
    Q flags=R<Q>(object,0);
    if(flags&1)return;
    W(object,0,flags|1);
    if(type==4)return;
    Q count=R<Q>(gc,0x88),capacity=R<Q>(gc,0x90);
    if(count==capacity){
        Expand(gc,0x80,capacity,32,16,GC_ValueBytes);
        W(gc,0x90,capacity?capacity*2:32);count=R<Q>(gc,0x88);
    }
    W(R<B*>(gc,0x80),unsigned(count*16),*v);W(gc,0x88,count+1);
}
// 1406922C0
void BarrierObject(B*gc,B*object){
    if(!(object[0]&1))return;
    Q cap=R<Q>(gc,0xa0),count=R<Q>(gc,0xa8);
    if(cap==count){
        Expand(gc,0x98,cap,8,8,GC_ObjectBytes);
        W(gc,0xa0,cap?cap*2:8);count=R<Q>(gc,0xa8);
    }
    W(R<B*>(gc,0x98),unsigned(count*8),object);W(gc,0xa8,count+1);
}
// 14068F740
void CollectStep(B*gc,void*vm,int cost){
    int left=int(R<U>(gc,0x140)-U(cost));W(gc,0x140,left);
    if(R<Q>(gc,0x138)>=R<Q>(R<B*>(gc,0x58),0x18)||left>0||gc[0x128])return;
    W(gc,0x140,0x13d);
    Interrupt callback=R<Interrupt>(gc,0x130);
    if(callback&&callback(R<void*>(gc,0x48)))return;
    W(gc,4,R<U>(gc,0x118)*R<U>(gc,0x3c)*0x13du);GC_Advance(gc,vm);
}
// 14068F7D0
U CollectFull(B*gc,void*vm,bool releaseScratch){
    Interrupt callback=R<Interrupt>(gc,0x130);
    if(callback&&callback(R<void*>(gc,0x48)))return 0;
    void*scratch=R<void*>(gc,0x60);
    if(releaseScratch&&scratch){
        Release(R<B*>(gc,0x58),scratch,R<Q>(gc,0x120),GC_ScratchBytes);
        W<void*>(gc,0x60,nullptr);
    }
    U previous=R<U>(gc,0);W(gc,0,0x80000000u);
    U result=GC_Advance(gc,vm);W(gc,0,previous);
    if(!GC_Check(gc))GC_OutOfMemory(R<void*>(gc,0x48));
    return result;
}
}
#endif
