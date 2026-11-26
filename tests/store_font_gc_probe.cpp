#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/lui_gc_store.cpp"
#undef T6_SOURCE
#include "../src/ui/lui_metatable_store.cpp"
#include <cstdlib>
#define MS __attribute__((ms_abi))
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>static void Write(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
template<class T>static T Read(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
static B gc[0x148],globals[0x400],vm[0x100],parent[16],object[32],values[512],objects[64],scratch[64],allocation[2048],stack[256],tables[4][64],nodes[4][160];
struct Capture{Q result,used,peak,flags,count1,cap1,count2,cap2;U valueBytes,objectBytes,scratchBytes,steps,checks,allocs,frees,interrupts,threshold,budget,status,stackSize;store_lui_meta::Value value;B list[64];};
static Capture*c;static bool veto,checkOk;static U advance;
namespace store_lui_gc{
extern "C"{
U GC_ValueBytes,GC_ObjectBytes,GC_ScratchBytes;
U GC_Advance(B*,void*){++c->steps;return advance;}
bool GC_Check(B*){++c->checks;return checkOk;}
void GC_OutOfMemory(void*){std::abort();}
}
}
namespace store_lui_meta{
extern "C"{
Value Meta_Nil{};
const char* Meta_TypeNames[16]{};
void Meta_Error(void*,const char*,...){std::abort();}
void Meta_GrowStack(void*,void*,int){std::abort();}
void Meta_Call(void*state,int,int,Q){B*p=Read<B*>(state,0x48)-48;Value value=Read<Value>(p+32,0);Write(p,0,value);Write(state,0x48,p+16);++c->steps;}
}
}
static void* Alloc(void*,void*p,Q,Q n){if(!n){++c->frees;return nullptr;}++c->allocs;if(p)std::abort();return allocation;}
static int Interrupt(void*){++c->interrupts;return veto;}
static void*MS MAlloc(void*a,void*p,Q o,Q n){return Alloc(a,p,o,n);}
static int MS MInterrupt(void*p){return Interrupt(p);}
static U MS MAdvance(B*p,void*v){return store_lui_gc::GC_Advance(p,v);}
static bool MS MCheck(B*p){return store_lui_gc::GC_Check(p);}
static void MS MCall(void*p,int a,int b,Q m){store_lui_meta::Meta_Call(p,a,b,m);}
static void Entry(unsigned table,store_lui_meta::Value k,store_lui_meta::Value val){
    auto*n=nodes[table]+32;Q h=(((k.payload>>3^k.payload)>>5)^U(k.payload))&3;
    Write(n,unsigned(h*32),k);Write(n,unsigned(h*32+16),val);
}
extern "C"{
void*GCMock(unsigned i){void*p[]={reinterpret_cast<void*>(MAdvance),reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MCall)};return p[i];}
unsigned GCSize(){return sizeof(Capture);}
void GCRun(void*image,unsigned op,unsigned seed,void*out){
    auto*b=static_cast<B*>(image);Capture cap{};c=&cap;U st=seed;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    std::memset(gc,0,sizeof gc);std::memset(globals,0,sizeof globals);std::memset(vm,0,sizeof vm);std::memset(values,0,sizeof values);std::memset(objects,0,sizeof objects);std::memset(allocation,0,sizeof allocation);std::memset(tables,0,sizeof tables);std::memset(nodes,0,sizeof nodes);
    Write(vm,0x10,globals);Write(vm,0x48,stack);Write(vm,0x58,stack+sizeof stack);
    Write(globals,0,b?reinterpret_cast<void*>(MAlloc):reinterpret_cast<void*>(Alloc));Write(globals,24,Q(4096));Write(globals,32,Q(8192));
    Write(gc,0x48,vm);Write(gc,0x58,globals);Write(gc,0x80,values);Write(gc,0x98,objects);
    Q capacity=seed&1?4:0,count=seed&2?capacity:capacity?1:0;
    if(!capacity){Write<B*>(gc,0x80,nullptr);Write<B*>(gc,0x98,nullptr);}
    Write(gc,0x88,count);Write(gc,0x90,capacity);Write(gc,0xa0,capacity);Write(gc,0xa8,count);
    Write(parent,0,Q(next()%2));Write(object,0,Q(next()%2));Write(object,8,Q(seed&4?~Q(0):0));
    Write(gc,0x140,U(next()%500));Write(gc,0x138,Q(next()%8192));gc[0x128]=B(next()%2);Write(gc,0x118,next()%8);Write(gc,0x3c,next()%8);
    if(seed&8)Write(gc,0x130,b?reinterpret_cast<void*>(MInterrupt):reinterpret_cast<void*>(Interrupt));
    if(seed&16){Write(gc,0x60,scratch);Write(gc,0x120,Q(sizeof scratch));}
    Write(gc,0,next());advance=next();veto=seed&32;checkOk=true;
    U initial=128;store_lui_gc::GC_ValueBytes=store_lui_gc::GC_ObjectBytes=store_lui_gc::GC_ScratchBytes=initial;
    if(b){Write(b,0x43c49b4,initial);Write(b,0x43c49b8,initial);Write(b,0x43c49ac,initial);}
    store_lui_gc::Value v{4+next()%7,0,reinterpret_cast<Q>(object)};
    if(op==0){if(b)reinterpret_cast<void(MS*)(B*,B*,const store_lui_gc::Value*)>(b+0x692140)(gc,parent,&v);else store_lui_gc::BarrierValue(gc,parent,&v);}
    else if(op==1){if(b)reinterpret_cast<void(MS*)(B*,B*)>(b+0x6922c0)(gc,object);else store_lui_gc::BarrierObject(gc,object);}
    else if(op==2){int cost=int(next()%700);if(b)reinterpret_cast<void(MS*)(B*,void*,int)>(b+0x68f740)(gc,vm,cost);else store_lui_gc::CollectStep(gc,vm,cost);}
    else if(op==3){bool release=seed&64;if(b)cap.result=reinterpret_cast<U(MS*)(B*,void*,bool)>(b+0x68f7d0)(gc,vm,release);else cap.result=store_lui_gc::CollectFull(gc,vm,release);}
    else{
        using V=store_lui_meta::Value;
        V key{4,0,Q(0x12345678)},idx{4,0,Q(0x10203040)},val{3,0,Q(next())},target{5,0,reinterpret_cast<Q>(tables[0])},method{5,0,reinterpret_cast<Q>(tables[1])};
        for(unsigned i=0;i<4;++i){Write(tables[i],0x18,4u-1);Write(tables[i],0x20,nodes[i]+32);}
        Write(globals,0x208,idx.payload);
        if(seed&1)Entry(0,key,val);
        if(seed&2)Entry(1,key,val);
        if(seed&4){Write(tables[0],0x10,tables[2]);Entry(2,idx,method);}
        if(seed&8){Write(tables[1],0x10,tables[3]);Entry(3,idx,V{9,0,0x4455});}
        if(op==4){if(b)reinterpret_cast<V*(MS*)(V*,B*,const V*,const V*)>(b+0x67efc0)(&cap.value,vm,&target,&key);else store_lui_meta::GetTable(&cap.value,vm,&target,&key);}
        if(op==5){if(b)reinterpret_cast<V*(MS*)(V*,B*,const V*,const V*,Q,const V*,int)>(b+0x693c10)(&cap.value,vm,&target,&key,0,&method,32);else store_lui_meta::ResolveIndex(&cap.value,vm,&target,&key,0,&method,32);}
        if(op==6){V obj{7,0,reinterpret_cast<Q>(object)};Write(object,0x10,tables[2]);Entry(2,idx,method);if(b)reinterpret_cast<V*(MS*)(V*,B*,const V*,const V*,Q,int)>(b+0x6939d0)(&cap.value,vm,&obj,&key,0,32);else store_lui_meta::IndexNonTable(&cap.value,vm,&obj,&key,0,32);}
        cap.stackSize=U(Read<B*>(vm,0x48)-stack);
    }
    cap.used=Read<Q>(globals,24);cap.peak=Read<Q>(globals,32);cap.flags=Read<Q>(object,0);cap.count1=Read<Q>(gc,0x88);cap.cap1=Read<Q>(gc,0x90);cap.count2=Read<Q>(gc,0xa8);cap.cap2=Read<Q>(gc,0xa0);
    cap.valueBytes=b?Read<U>(b,0x43c49b4):store_lui_gc::GC_ValueBytes;cap.objectBytes=b?Read<U>(b,0x43c49b8):store_lui_gc::GC_ObjectBytes;cap.scratchBytes=b?Read<U>(b,0x43c49ac):store_lui_gc::GC_ScratchBytes;
    cap.threshold=Read<U>(gc,0x140);cap.budget=Read<U>(gc,4);cap.status=Read<U>(gc,0);
    if(op==0&&cap.count1&&Read<B*>(gc,0x80))std::memcpy(cap.list,Read<B*>(gc,0x80)+(cap.count1-1)*16,16);
    if(op==1&&cap.count2&&Read<B*>(gc,0x98))std::memcpy(cap.list,Read<B*>(gc,0x98)+(cap.count2-1)*8,8);
    std::memcpy(out,&cap,sizeof cap);
}
}
