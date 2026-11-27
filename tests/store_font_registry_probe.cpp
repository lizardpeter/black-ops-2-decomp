#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/lui_registry_store.cpp"
#include <cstdio>
#include <cstdlib>
#define MS __attribute__((ms_abi))
namespace store_lui_registry{
static B lua[0x120],globals[0x600],table[0x50],meta[0x50],fn[0xc0],userdata[256],stack[32*16],array[16*16],hash[8*40],allocation[512];
static int allocCalls,stepCalls,fullCalls,resizeCalls,barrierObjectCalls,barrierValueCalls,insertCalls;
static bool failFirst;static int lengthValue;
extern "C"{
Value Registry_Nil;
static B newTable[64],newHash[40*256],newArray[16*64];
static Q newHashSize,newArraySize,stringLength;
static int setTableIndex,growCount;
static Value setKey,setValue;
void Registry_GrowStack(B*,B*l,int n){growCount+=n;W(l,0x58,stack+sizeof stack);}
void Registry_PushString(B*l,const char*s,Q len){stringLength=len;Push(l,Value{4,0,reinterpret_cast<Q>(s)});}
void Registry_SetTable(B*l,int idx){setTableIndex=idx;setKey=R<Value>(Top(l)-32,0);setValue=R<Value>(Top(l)-16,0);W(l,0x48,Top(l)-32);}
void Registry_AllocTable(B*,B*,Q h,U a,void**p){
    newHashSize=h;newArraySize=a;std::memset(newTable,0xa5,sizeof newTable);std::memset(newHash,0xa5,sizeof newHash);std::memset(newArray,0xa5,sizeof newArray);
    p[0]=newTable;p[1]=h?newHash:nullptr;p[2]=a?newArray:nullptr;
}
int Registry_AllocationTotals[64];
void Registry_ResizeTable(B*t,B*,U n,U){++resizeCalls;W(t,0x30,n);}
void Registry_BarrierObject(B*,B*){++barrierObjectCalls;}
void Registry_BarrierValue(B*,B*,const Value*){++barrierValueCalls;}
int Registry_FormatNumber(char*d,U n,const char*fmt,double v){return std::snprintf(d,n,fmt,v);}
U Registry_TableLength(B*){return U(lengthValue);}
void Registry_InsertHash(B*,B*,const Value*,const Value*){++insertCalls;}
void Registry_CollectStep(B*,B*,int){++stepCalls;}
void Registry_CollectFull(B*,B*,bool){++fullCalls;}
void Registry_OutOfMemory(B*,Q){std::abort();}
}
static void* NativeAlloc(void*,void*,Q,Q){++allocCalls;if(failFirst&&allocCalls==1)return nullptr;return allocation;}
static void* MS MAlloc(void*a,void*b,Q c,Q d){return NativeAlloc(a,b,c,d);}
static void MS MResize(B*t,B*l,U n,U h){Registry_ResizeTable(t,l,n,h);}
static void MS MBarrierObject(B*a,B*b){Registry_BarrierObject(a,b);}
static void MS MBarrierValue(B*a,B*b,const Value*c){Registry_BarrierValue(a,b,c);}
static int MS MFormat(char*d,U n,const char*f,double v){return Registry_FormatNumber(d,n,f,v);}
static U MS MLength(B*t){return Registry_TableLength(t);}
static void MS MInsert(B*t,B*l,const Value*k,const Value*v){Registry_InsertHash(t,l,k,v);}
static void MS MStep(B*g,B*l,int n){Registry_CollectStep(g,l,n);}
static void MS MFull(B*g,B*l,bool b){Registry_CollectFull(g,l,b);}
static void MS MOOM(B*l,Q n){Registry_OutOfMemory(l,n);}
static void MS MGrow(B*p,B*l,int n){Registry_GrowStack(p,l,n);}
static void MS MPushString(B*l,const char*s,Q n){Registry_PushString(l,s,n);}
static void MS MSetTable(B*l,int n){Registry_SetTable(l,n);}
static void MS MAllocTable(B*a,B*b,Q h,U s,void**p){Registry_AllocTable(a,b,h,s,p);}
static void* MS MZero(void*p,int n,Q s){return std::memset(p,n,s);}
static Q Bits(float f){U u;std::memcpy(&u,&f,4);return u;}
static Value V(U t,Q p){return {t,0,p};}
static Value P(U t,void*p){return V(t,reinterpret_cast<Q>(p));}
struct Capture{
    Q result,used,peak,head,header,environment,metatable,next;
    int count,top,totals,allocCalls,stepCalls,fullCalls,resizeCalls,barrierObjectCalls,barrierValueCalls,insertCalls;
    Value values[16],topValue;
    Q hSize,aSize,strLen;int fieldIndex,grows;Value fieldKey,fieldValue;B tableBytes[64];U hashNonzero,arrayNonzero;
};
static void Reset(unsigned seed){
    std::memset(lua,0,sizeof lua);std::memset(globals,0,sizeof globals);std::memset(table,0,sizeof table);std::memset(meta,0,sizeof meta);std::memset(fn,0,sizeof fn);std::memset(userdata,0,sizeof userdata);std::memset(stack,0,sizeof stack);std::memset(array,0,sizeof array);std::memset(hash,0,sizeof hash);std::memset(allocation,0,sizeof allocation);std::memset(Registry_AllocationTotals,0,sizeof Registry_AllocationTotals);
    W(lua,0x10,globals);W(lua,0x48,stack+8*16);W(lua,0x50,stack+2*16);W(lua,0x18,stack);W(lua,0x28,stack+24);
    W(lua,0x78,Q(0x11223344));W(stack,16,P(9,fn));W(fn,0x18,Q(0x55667788));
    W(table,0x18,7u);W(table,0x20,hash+8*8);W(table,0x28,array);W(table,0x30,8u);
    W(globals,0x1e0,P(5,table));W(lua,0x70,P(5,table));W(globals,0x1e8,table);
    W(globals,0x18,Q(1000));W(globals,0x20,Q(1200));W(globals,0x10,1u);
    Registry_Nil=V(0,0);
    for(unsigned i=0;i<16;++i)W(array,i*16,V(i%2?2:3,Q(0x44400000u+i)));
    W(userdata,0,Q(128)<<10);W(userdata,8,23u);
    allocCalls=stepCalls=fullCalls=resizeCalls=barrierObjectCalls=barrierValueCalls=insertCalls=0;
    failFirst=(seed&8)!=0;lengthValue=4;
    newHashSize=newArraySize=stringLength=0;setTableIndex=growCount=0;setKey=setValue={};
}
extern "C"{
unsigned RegistrySize(){return sizeof(Capture);}
void*RegistryMock(unsigned i){
    void*p[]={reinterpret_cast<void*>(MResize),reinterpret_cast<void*>(MBarrierObject),reinterpret_cast<void*>(MBarrierValue),reinterpret_cast<void*>(MFormat),reinterpret_cast<void*>(MLength),reinterpret_cast<void*>(MInsert),reinterpret_cast<void*>(MStep),reinterpret_cast<void*>(MFull),reinterpret_cast<void*>(MOOM),reinterpret_cast<void*>(MGrow),reinterpret_cast<void*>(MPushString),reinterpret_cast<void*>(MSetTable),reinterpret_cast<void*>(MAllocTable),reinterpret_cast<void*>(MZero)};
    return p[i];
}
void RegistryRun(void*image,unsigned op,unsigned seed,void*out){
    Capture c{};Reset(seed);B*b=static_cast<B*>(image);
    W(globals,0,b?reinterpret_cast<Q>(MAlloc):reinterpret_cast<Q>(NativeAlloc));
    if(b){W(b,0x265e618,Registry_Nil);for(unsigned i=0;i<64;++i)W(b,0x43c4950+i*4,0);}
    int idxs[]={1,-1,2,3,0,-9999,10,-10000,-10001,-10002,-10003};int idx=idxs[seed%11];
    U key=seed%12+1;U type=(seed>>4)%12;
    Value value=V(2,Q(seed));Value k=V(3,Bits(float(key)));
    const int phase=(seed>>8)%5;W(globals,0xc0,phase);
    W(globals,0x1b0,Q(seed&1));
    if(op==0||op==6){
        Value v=type==7?P(7,userdata):type==4?P(4,userdata):type==5?P(5,table):type==2?P(2,userdata+64):type==3?V(3,Bits(float(int(seed%2000)-1000)*0.125f)):V(type,0);
        W(stack,2*16,v);W(stack,7*16,v);W(stack,3*16,v);W(fn,3*16,v);
        if(op==0)c.result=b?reinterpret_cast<Q>(reinterpret_cast<void*(MS*)(B*,int)>(b+0x662630)(lua,idx)):reinterpret_cast<Q>(ToUserData(lua,idx));
        else c.result=b?reinterpret_cast<Q(MS*)(B*,int)>(b+0x663dc0)(lua,idx):ObjectLength(lua,idx);
    }else if(op==1){
        if(key>8){B*p=hash+64+(key&7)*32;W(p,0,k);W(p,16,value);}
        if(b)reinterpret_cast<void(MS*)(B*,int,U)>(b+0x663840)(lua,-10000,key);else RawGetI(lua,-10000,key);
    }else if(op==2)c.result=b?reinterpret_cast<Q(MS*)(B*,int,U)>(b+0x663ad0)(lua,-10000,key):RawLightUserData(lua,-10000,key);
    else if(op==3){
        if(b)reinterpret_cast<void(MS*)(B*,int,U,Q)>(b+0x663ba0)(lua,-10000,key,value.payload);else RawSetLightUserData(lua,-10000,key,value.payload);
    }else if(op==4){
        Value t=P(5,table);if(b)reinterpret_cast<void(MS*)(B*,const Value*,U,const Value*)>(b+0x67f200)(lua,&t,key,&value);else SetArrayValue(lua,reinterpret_cast<B*>(&t),key,&value);
    }else if(op==5){
        if(b)reinterpret_cast<void(MS*)(B*,B*,const Value*,const Value*)>(b+0x6812c0)(table,lua,&k,&value);else AssignTableValue(table,lua,&k,&value);
    }else if(op==7){
        Value obj=P((seed&1)?5:7,(seed&1)?table:userdata),m=(seed&2)?P(5,meta):V(0,0);
        c.result=b?reinterpret_cast<int(MS*)(B*,const Value*,const Value*)>(b+0x67ef40)(lua,&obj,&m):SetMetaValue(lua,&obj,&m);
    }else if(op==8){
        W(array,0,V(2,(seed&1)?3:0));W(array,3*16,V(2,1));W(stack,7*16,(seed&2)?V(0,0):P(7,userdata));
        c.result=Q(static_cast<long long>(b?reinterpret_cast<int(MS*)(B*,int)>(b+0x69bb00)(lua,-10000):Reference(lua,-10000)));
    }else if(op==9){
        W(array,0,V(2,3));int ref=(seed&1)?int(key): -1;
        if(b)reinterpret_cast<void(MS*)(B*,int,int)>(b+0x69bd00)(lua,-10000,ref);else Unreference(lua,-10000,ref);
    }else if(op==10){
        Q size=seed%128;
        c.result=reinterpret_cast<Q>(b?reinterpret_cast<B*(MS*)(B*,Q,int)>(b+0x692970)(lua,size,27):AllocateObject(lua,size,27));
    }else if(op==11){
        Q size=seed%128;
        c.result=reinterpret_cast<Q>(b?reinterpret_cast<void*(MS*)(B*,Q)>(b+0x69ea10)(lua,size):NewUserData(lua,size));
    }else if(op==13){
        int valid[]={1,2,3,-1,-2,-10000,-10001,-10002,-10003};
        int i=valid[seed%9];W(stack,2*16,value);W(stack,7*16,value);W(fn,3*16,value);
        if(b)reinterpret_cast<void(MS*)(B*,int)>(b+0x218c80)(lua,i);else PushValue(lua,i);
    }else if(op==14){
        const char*keyName=(seed&1)?"UIQuadCache":nullptr;
        W(stack,7*16,value);W(lua,0x58,Top(lua)+(seed&2?16:48));
        if(b)reinterpret_cast<void(MS*)(B*,int,const char*)>(b+0x218de0)(lua,idx,keyName);else SetField(lua,idx,keyName);
        c.hSize=newHashSize;c.aSize=newArraySize;c.strLen=stringLength;c.fieldIndex=setTableIndex;c.grows=growCount;c.fieldKey=setKey;c.fieldValue=setValue;
    }else if(op==15){
        int i=(seed&2)?1:0;
        W(stack,2*16,P((seed&1)?5:7,(seed&1)?table:userdata));W(stack,7*16,(seed&4)?P(5,meta):V(0,0));
        c.result=b?reinterpret_cast<int(MS*)(B*,int)>(b+0x218ea0)(lua,i):SetMetatable(lua,i);
    }else if(op==16){
        unsigned n=seed%64;int h=int((seed>>9)%256);
        B*t=b?reinterpret_cast<B*(MS*)(B*,U,int)>(b+0x681180)(lua,n,h):NewTable(lua,n,h);
        c.result=reinterpret_cast<Q>(t);c.hSize=newHashSize;c.aSize=newArraySize;std::memcpy(c.tableBytes,t,64);
        for(Q i=0;i<newHashSize;++i)c.hashNonzero+=newHash[i]!=0;
        for(Q i=0;i<newArraySize;++i)c.arrayNonzero+=newArray[i]!=0;
    }else{
        Value obj=type==7?P(7,userdata):type==4?P(4,userdata):type==5?P(5,table):type==3?V(3,Bits(float(int(seed%1000)-500)*0.25f)):V(type,0);
        c.result=b?reinterpret_cast<Q(MS*)(B*,const Value*)>(b+0x67f290)(lua,&obj):ValueLength(lua,&obj);
    }
    c.used=R<Q>(globals,0x18);c.peak=R<Q>(globals,0x20);c.head=R<Q>(globals,0x1f0);c.header=R<Q>(allocation,0);c.environment=R<Q>(allocation,8);c.metatable=(seed&1)?R<Q>(table,0x10):R<Q>(userdata,0x10);c.next=R<Q>(allocation,0x18);
    c.count=R<U>(table,0x30);c.top=int((Top(lua)-stack)/16);c.totals=b?R<int>(b,0x43c4950+27*4):Registry_AllocationTotals[27];
    c.allocCalls=allocCalls;c.stepCalls=stepCalls;c.fullCalls=fullCalls;c.resizeCalls=resizeCalls;c.barrierObjectCalls=barrierObjectCalls;c.barrierValueCalls=barrierValueCalls;c.insertCalls=insertCalls;
    for(unsigned i=0;i<16;++i){c.values[i]=R<Value>(array,i*16);c.values[i].aux=0;}c.topValue=R<Value>(Top(lua)-16,0);c.topValue.aux=0;
    // The original leaves undefined auxiliary lanes when constructing a light or full userdata.
    std::memcpy(out,&c,sizeof c);
}
}
}
