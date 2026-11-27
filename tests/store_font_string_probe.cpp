#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/lui_string_store.cpp"
#undef T6_SOURCE
#include "../src/qcommon/string_lookup_store.cpp"
#include <cstdlib>
#define MS __attribute__((ms_abi))
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
using store_string_lookup::Bucket;
static Bucket buckets[8];
static B pool[24*8],vm[128],globals[512],stack[32],str[128];
static U home,lockCount,unlockCount,compareCount,hashCount,internCount,hashValue,internFlag;
static Q internLength;
template<class T>static void Put(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
template<class T>static T Get(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
namespace store_string_lookup{
extern "C"{
Bucket* StringLookup_Buckets=buckets;const B* StringLookup_Pool=pool;
U StringLookup_Hash(const void*,U){++hashCount;return home;}
void StringLookup_Lock(){++lockCount;}
void StringLookup_Unlock(){++unlockCount;}
int StringLookup_Compare(const void*a,const void*b,U n){++compareCount;return std::memcmp(a,b,n);}
}}
namespace store_lui_string{
extern "C" Q String_InternInternal(B*p,B*v,U flag,const B*t,Q n,U hash){
    if(p!=globals+0x1c0||v!=vm||t!=str)std::abort();
    ++internCount;internFlag=flag;internLength=n;hashValue=hash;return 0x123456789abcULL;
}}
static U MS MHash(const void*p,U n){return store_string_lookup::StringLookup_Hash(p,n);}
static void MS MLock(void*){store_string_lookup::StringLookup_Lock();}
static void MS MUnlock(void*){store_string_lookup::StringLookup_Unlock();}
static int MS MCompare(const void*a,const void*b,Q n){return store_string_lookup::StringLookup_Compare(a,b,U(n));}
static Q MS MIntern(B*p,B*v,U f,const B*t,Q n,U h){return store_lui_string::String_InternInternal(p,v,f,t,n,h);}
struct Capture{Q result,length,handle,stackDelta;U locks,unlocks,compares,hashes,interns,hash,flag,tag;Bucket chain[8];};
extern "C"{
unsigned StringSize(){return sizeof(Capture);}
void* StringMock(unsigned i){void*p[]={reinterpret_cast<void*>(MHash),reinterpret_cast<void*>(MLock),reinterpret_cast<void*>(MUnlock),reinterpret_cast<void*>(MCompare),reinterpret_cast<void*>(MIntern)};return p[i];}
void StringRun(void*image,unsigned op,U seed,void*out){
    auto*b=static_cast<B*>(image);Capture c{};U s=seed;
    auto next=[&](){s=s*1664525u+1013904223u;return s;};
    home=1;lockCount=unlockCount=compareCount=hashCount=internCount=hashValue=internFlag=0;internLength=0;
    std::memset(buckets,0,sizeof buckets);std::memset(pool,0,sizeof pool);std::memset(vm,0,sizeof vm);std::memset(stack,0,sizeof stack);
    if(op==0){
        U n=next()%17;
        for(U i=1;i<=5;++i){
            buckets[i]={0x10000u|(i==5?1u:i+1),i};
            pool[i*24+3]=B(n);
            for(U j=0;j<n;++j)pool[i*24+4+j]=B(next()>>24);
            if(n)pool[i*24+4]=B(i);
        }
        buckets[1].flagsAndNext=(seed&1?0x20000u:0x10000u)|2u;
        U pick=1+next()%5;std::memcpy(str,pool+pick*24+4,n);
        if(seed&2)str[0]=99;
        if(seed&4)pool[pick*24+3]=B(n+1);
        if(b){Put(b,0xa56900,static_cast<Bucket*>(buckets));Put(b,0x2275610,static_cast<B*>(pool));}
        c.result=b?reinterpret_cast<U(MS*)(const void*,U)>(b+0x50a9b0)(str,n):store_string_lookup::FindExistingString(str,n);
        std::memcpy(c.chain,buckets,sizeof buckets);
    }else{
        Q n=next()%128;for(auto&x:str)x=B(next()>>24);
        Put(vm,0x10,static_cast<B*>(globals));Put(vm,0x48,static_cast<B*>(stack));
        if(b)reinterpret_cast<void(MS*)(B*,const B*,Q)>(b+0x126780)(vm,str,n);else store_lui_string::PushString(vm,str,n);
        c.tag=Get<U>(stack,0);c.handle=Get<Q>(stack,8);c.stackDelta=Get<B*>(vm,0x48)-stack;
    }
    c.length=internLength;c.locks=lockCount;c.unlocks=unlockCount;c.compares=compareCount;c.hashes=hashCount;c.interns=internCount;c.hash=hashValue;c.flag=internFlag;
    std::memcpy(out,&c,sizeof c);
}
}
