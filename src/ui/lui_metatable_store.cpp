// Original Store HKS hash lookup and __index metamethod dispatch.
#define T6_SOURCE "ui/lui_hks.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_lui_meta{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
struct Value{U tag,aux;Q payload;};
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern Value Meta_Nil;
extern const char* Meta_TypeNames[16];
void Meta_Error(void*,const char*,...);
void Meta_GrowStack(void*,void*,int);
void Meta_Call(void*,int,int,Q);
}
static unsigned Tag(const Value&v){return v.tag&15;}
static B* Ptr(Q p){return reinterpret_cast<B*>(p);}
static Value Raw(B*table,const Value&key){
    B*nodes=R<B*>(table,0x20);
    if(!nodes)return Meta_Nil;
    Q k=key.payload;
    Q index=(((k>>3^k)>>5)^U(k))&R<U>(table,0x18);
    B*p=nodes+index*32;
    do{
        Value current=R<Value>(p,0);
        if(Tag(current)==Tag(key)&&current.payload==k)return R<Value>(p,16);
        p=R<B*>(nodes-((p-nodes)/32+1)*8,0);
    }while(p);
    return Meta_Nil;
}
static Value IndexMethod(B*vm,B*mt){
    return Raw(mt,Value{4,0,R<Q>(R<B*>(vm,0x10),0x208)});
}
Value* ResolveIndex(Value*,B*,const Value*,const Value*,Q,const Value*,int);
// 1406939D0
Value* IndexNonTable(Value*out,B*vm,const Value*object,const Value*key,Q mode,int limit){
    U type=Tag(*object);B*meta=nullptr;
    if(type==5||type==7)meta=R<B*>(Ptr(object->payload),0x10);
    else{
        B*globals=R<B*>(vm,0x10);
        Value found=Raw(R<B*>(globals,0x1e8),Value{4,0,R<Q>(globals,(type+0x32)*16)});
        if(Tag(found))meta=Ptr(found.payload);
    }
    if(!meta)Meta_Error(vm,"attempt to index a %s value",Meta_TypeNames[type]);
    Value method=IndexMethod(vm,meta);
    if(!Tag(method))Meta_Error(vm,"attempt to index a %s value",Meta_TypeNames[type]);
    return ResolveIndex(out,vm,object,key,mode,&method,limit);
}
// 140693C10
Value* ResolveIndex(Value*out,B*vm,const Value*object,const Value*key,Q mode,const Value*method,int limit){
    U type=Tag(*method);
    if(type==9||type==10){
        B*top=R<B*>(vm,0x48);
        if(R<B*>(vm,0x58)<top+48){Meta_GrowStack(vm+0x18,vm,3);top=R<B*>(vm,0x48);}
        W(top,0,*method);W(top,16,*object);W(top,32,*key);W(vm,0x48,top+48);
        Meta_Call(vm,2,1,mode);top=R<B*>(vm,0x48)-16;
        *out=R<Value>(top,0);W(vm,0x48,top);return out;
    }
    if(!limit)Meta_Error(vm,"loop in gettable");
    if(type!=5)return IndexNonTable(out,vm,method,key,mode,limit-1);
    B*table=Ptr(method->payload);
    Value value=Raw(table,*key);
    if(Tag(value)){*out=value;return out;}
    *out=Meta_Nil;
    if(B*meta=R<B*>(table,0x10)){
        Value next=IndexMethod(vm,meta);
        if(Tag(next))return ResolveIndex(out,vm,method,key,mode,&next,limit-1);
    }
    return out;
}
// 14067EFC0
Value* GetTable(Value*out,B*vm,const Value*object,const Value*key){
    if(Tag(*object)!=5)return IndexNonTable(out,vm,object,key,0,32);
    B*table=Ptr(object->payload);
    Value found=Raw(table,*key);
    if(Tag(found)){*out=found;return out;}
    *out=Meta_Nil;
    if(B*mt=R<B*>(table,0x10)){
        Value next=IndexMethod(vm,mt);
        if(Tag(next))return ResolveIndex(out,vm,object,key,0,&next,32);
    }
    return out;
}
}
#endif
