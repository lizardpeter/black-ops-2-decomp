// Store MP HKS registry and userdata operations used by the UI quad cache.
#define T6_SOURCE "lui/lui_registry.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_lui_registry {
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
struct Value{U tag,aux;Q payload;};
static_assert(sizeof(Value)==16,"Store value layout");
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern Value Registry_Nil;
void Registry_ResizeTable(B*,B*,U,U);
void Registry_BarrierObject(B*,B*);
void Registry_BarrierValue(B*,B*,const Value*);
int Registry_FormatNumber(char*,U,const char*,double);
U Registry_TableLength(B*);
void Registry_InsertHash(B*,B*,const Value*,const Value*);
void Registry_CollectStep(B*,B*,int);
void Registry_CollectFull(B*,B*,bool);
void Registry_OutOfMemory(B*,Q);
extern int Registry_AllocationTotals[];
}
static B* Globals(B*l){return R<B*>(l,0x10);}
static B* Top(B*l){return R<B*>(l,0x48);}
static B* Base(B*l){return R<B*>(l,0x50);}
static B* Index(B*l,int idx,bool checked){
    if(idx>=-9999){
        if(idx>0){B*p=Base(l)+(static_cast<long long>(idx)-1)*16;return checked&&p>=Top(l)?nullptr:p;}
        B*p=Top(l)+static_cast<long long>(idx)*16;
        return checked&&(!idx||p<Base(l))?nullptr:p;
    }
    if(idx==-10000)return Globals(l)+0x1e0;
    if(idx==-10002)return l+0x70;
    B*function=R<B*>(Base(l)-8,0);
    if(idx==-10001){W(l,0x88,R<Q>(function,0x18));W(l,0x80,5u);return l+0x80;}
    return function+(static_cast<long long>(-10003-idx)+3)*16;
}
static U Tag(const Value&v){return v.tag&15;}
static B*Object(const Value&v){return reinterpret_cast<B*>(v.payload);}
static bool IsMarked(B*l){return R<int>(Globals(l),0xc0)==1;}
static Value ArrayValue(B*table,U idx){
    if(idx-1<R<U>(table,0x30))return R<Value>(R<B*>(table,0x28)+(idx-1)*16,0);
    return Registry_Nil;
}
static void Push(B*l,const Value&v){B*p=Top(l);W(p,0,v);W(l,0x48,p+16);}
static const B* HashEntry(B*table,U bucket){
    const B*nodes=R<const B*>(table,0x20);
    return nodes?nodes+static_cast<Q>(bucket&R<U>(table,0x18))*32:nullptr;
}
static const B* NextEntry(B*table,const B*node){
    const B*nodes=R<const B*>(table,0x20);
    return R<const B*>(nodes-(static_cast<Q>(node-nodes)/32+1)*8,0);
}
// 140662630: checked API stack index and full/light userdata distinction.
void* ToUserData(B*l,int idx){
    const B*p=Index(l,idx,true);if(!p)return nullptr;
    Value v=R<Value>(p,0);
    if(Tag(v)==7)return Object(v)+0x20;
    if(Tag(v)==2)return Object(v);
    return nullptr;
}
// 140663840: raw numeric lookup, array first then chained hash nodes.
void RawGetI(B*l,int idx,U key){
    B*t=Object(R<Value>(Index(l,idx,false),0));
    if(key-1<R<U>(t,0x30)){Push(l,ArrayValue(t,key));return;}
    for(const B*p=HashEntry(t,key);p;p=NextEntry(t,p)){
        if((p[0]&15)==3&&R<float>(p,8)==float(static_cast<int>(key))){Push(l,R<Value>(p,16));return;}
    }
    Push(l,Registry_Nil);
}
// 140663AD0: registry freelist uses light userdata payloads in array slots.
Q RawLightUserData(B*l,int idx,U key){
    Value v=ArrayValue(Object(R<Value>(Index(l,idx,false),0)),key);
    return Tag(v)==2?v.payload:0;
}
// 14067F200: original table write and incremental GC barriers.
void SetArrayValue(B*l,const B*table,U key,const Value*value){
    B*t=Object(R<Value>(table,0));
    if(R<U>(t,0x30)<key){
        Registry_ResizeTable(t,l,key,R<U>(t,0x18)+1);
        if(IsMarked(l))Registry_BarrierObject(Globals(l)+0x58,t);
    }
    W(R<B*>(t,0x28)+(key-1)*16,0,*value);
    if(IsMarked(l))Registry_BarrierValue(Globals(l)+0x58,t,value);
}
// 140663BA0: raw integer-key light userdata set. Aux lane not semantically read.
void RawSetLightUserData(B*l,int idx,U key,Q payload){
    Value v{2,0,payload};SetArrayValue(l,Index(l,idx,false),key,&v);
}
// 14067F290: HKS value length dispatch.
Q ValueLength(B*,const Value*v){
    switch(Tag(*v)){
    case 3:{char s[264];float f;std::memcpy(&f,&v->payload,4);return static_cast<Q>(static_cast<long long>(Registry_FormatNumber(s,256,"%0.8g",double(f))));}
    case 4:return static_cast<Q>(static_cast<long long>(R<int>(Object(*v),8)));
    case 5:return Registry_TableLength(Object(*v));
    case 7:return (R<Q>(Object(*v),0)>>10)-0x20;
    default:return 0;
    }
}
// 140663DC0: checked stack value length.
Q ObjectLength(B*l,int idx){B*p=Index(l,idx,true);return p?ValueLength(l,reinterpret_cast<Value*>(p)):0;}
// 1406812C0: table insertion selects the array only for exact positive integers.
void AssignTableValue(B*t,B*l,const Value*key,const Value*v){
    float f;std::memcpy(&f,&key->payload,4);
    int n=_mm_cvttss_si32(_mm_set_ss(f));
    if(Tag(*key)==3&&float(n)==f&&n>0&&U(n-1)<R<U>(t,0x30))
        W(R<B*>(t,0x28)+U(n-1)*16,0,*v);
    else Registry_InsertHash(t,l,key,v);
    if(IsMarked(l))Registry_BarrierValue(Globals(l)+0x58,t,v);
}
// 14067EF40: metatable pointer or per-type metatable table.
int SetMetaValue(B*l,const Value*object,const Value*meta){
    U type=Tag(*object);
    if(type!=5&&type!=7){
        B*g=Globals(l);AssignTableValue(R<B*>(g,0x1e8),l,reinterpret_cast<const Value*>(g+0x308+(type+1)*16),meta);return 0;
    }
    B*t=Object(*object);W(t,0x10,Tag(*meta)?meta->payload:0);
    if(IsMarked(l))Registry_BarrierValue(Globals(l)+0x58,t,meta);
    return 0;
}
// 14069BB00: reference freelist at array index 1; zero denotes no freed slots.
int Reference(B*l,int idx){
    B*top=Top(l);
    if(top-16>=Base(l)&&(top[-16]&15)==0){W(l,0x48,top-16);return -1;}
    Value first=ArrayValue(Object(R<Value>(Index(l,idx,false),0)),1);
    int head=Tag(first)==2?int(first.payload):0;
    if(Tag(first)==2&&head){
        if(U(idx+9999)<10000)idx+=1+int((Top(l)-Base(l))/16);
        Q next=RawLightUserData(l,idx,head+1);
        RawSetLightUserData(l,idx,1,next);
    }else{
        if(Tag(first)!=2)RawSetLightUserData(l,idx,1,0);
        head=int(ObjectLength(l,idx));
    }
    SetArrayValue(l,Index(l,idx,false),head+1,reinterpret_cast<Value*>(Top(l)-16));
    W(l,0x48,Top(l)-16);return head+1;
}
// 14069BD00: remove reference and return it to the original freelist.
void Unreference(B*l,int idx,int ref){
    if(ref<0)return;
    if(U(idx+9999)<10000)idx+=1+int((Top(l)-Base(l))/16);
    Q old=RawLightUserData(l,idx,1);
    RawSetLightUserData(l,idx,U(ref),old);
    RawSetLightUserData(l,idx,1,Q(static_cast<long long>(ref)-1));
}
// 140692970: VM allocator retry, allocation category and accounting.
B* AllocateObject(B*l,Q size,int type){
    B*g=Globals(l);if(!size)return nullptr;
    Registry_CollectStep(g+0x58,l,1);
    using Alloc=void*(*)(void*,void*,Q,Q);
    B*p=nullptr;int retry=3;
    while(!p){
        --retry;p=static_cast<B*>(R<Alloc>(g,0)(R<void*>(g,8),nullptr,0,size));
        if(p)break;
        if(!retry){Registry_OutOfMemory(l,size);retry=3;}
        Registry_CollectFull(Globals(l)+0x58,l,true);
    }
    Registry_AllocationTotals[type]+=int(size);
    Q used=R<Q>(g,0x18)+size;W(g,0x18,used);
    if(used>R<Q>(g,0x20))W(g,0x20,used);
    W(p,0,((size<<8)|Q(static_cast<long long>(type)))<<2|Q(static_cast<long long>(R<int>(g,0x10))));
    return p;
}
// 14069EA10: full userdata header and GC list, leaves payload on VM stack.
void* NewUserData(B*l,Q bytes){
    B*p=AllocateObject(l,bytes+0x20,0x1b);
    W(p,0x10,Q(0));
    B*g=Globals(l);Q environment;
    if((R<B*>(l,0x28)-R<B*>(l,0x18))/0x18<0)environment=R<Q>(l,0x78);
    else environment=R<Q>(R<B*>(Base(l)-8,0),0x18);
    W(p,8,environment);W(p,0x18,R<Q>(g,0x1f0));W(g,0x1f0,p);
    Push(l,Value{7,0,reinterpret_cast<Q>(p)});
    int phase=R<int>(Globals(l),0xc0);
    if(phase==4||(phase==3&&R<Q>(Globals(l),0x1b0)!=0))W(p,0,R<Q>(p,0)&~Q(1));
    return p+0x20;
}
}
#endif
