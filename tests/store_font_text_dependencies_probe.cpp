#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/client/cl_text_bindings_store.cpp"
#undef T6_SOURCE
#include "../src/cgame/cg_cached_tag_store.cpp"
#include <cstdlib>
#define MS __attribute__((ms_abi))
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;using H=std::uint16_t;
template<class T>static void Put(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
template<class T>static T Get(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
static B modes[48],state[0x2ab0],records[8*0x120],entity[0x340],cache[0x90],gameState[0x48090],clientState[0x2cb4],transform[32],object[0x90],allocation[64];
static int keys[2*0x44a];static short indices[0x701];static char name[160];static bool absent;static U calls[10],flags;
namespace store_client_text{
extern "C"{
const B* TextBindings_ClientMode=modes;const int* TextBindings_KeyTable=keys;const B* TextBindings_ClientState=state;int TextBindings_ClientCount=2;
const char* TextBindings_Lookup(int,int,int n){if(n!=1)std::abort();++calls[0];return absent?nullptr:name;}
const char* TextBindings_Resolve(const void*,const void*,U f){++calls[1];flags=f;return absent?nullptr:name;}
void TextBindings_Sanitize(char*s){++calls[2];for(;*s;++s)if(*s=='!')*s='?';}
}}
namespace store_cached_tag{
extern "C"{
U CachedTag_Mode;H CachedTag_First=123,CachedTag_Second=456,CachedTag_Third=789;
const B* CachedTag_GameState=gameState;const short* CachedTag_ObjectIndices=indices;const B* CachedTag_Objects=object;
const B* CachedTag_ClientState=clientState;int CachedTag_ActiveClient=0,CachedTag_LastFailureGeneration;
U CachedTag_Fallback(B*,U tag,float*out){++calls[3];out[0]=float(tag);out[1]=17;out[2]=23;return 91;}
const B* CachedTag_GetTransform(B*,const B*,U){++calls[4];return absent?nullptr:transform;}
void CachedTag_Error(int,const char*){std::abort();}
void CachedTag_Yield(){std::abort();}
bool CachedTag_TryLock(B*p){if(Get<U>(p,0))return false;Put(p,0,1u);return true;}
void* CachedTag_Allocate(U n){++calls[5];flags=n;return absent?nullptr:allocation;}
void CachedTag_BuildMask(B*,int part,B*mask){++calls[6];std::memset(mask,0,32);Put(mask,0,part);}
void CachedTag_Calculate(void*,B*,B*){++calls[7];}
void CachedTag_Finalize(B*p,B*mask){++calls[8];int part=Get<int>(mask,0);unsigned off=0x40+unsigned(part/32)*4;Put(p,off,Get<U>(p,off)|(0x80000000u>>(part&31)));}
}}
static const char* MS MLookup(int a,int b,int c){return store_client_text::TextBindings_Lookup(a,b,c);}
static const char* MS MResolve(const void*a,const void*b,U f){return store_client_text::TextBindings_Resolve(a,b,f);}
static void MS MSanitize(char*s){store_client_text::TextBindings_Sanitize(s);}
static U MS MFallback(B*p,U t,float*out){return store_cached_tag::CachedTag_Fallback(p,t,out);}
static const B* MS MTransform(B*p,const B*q,U t){return store_cached_tag::CachedTag_GetTransform(p,q,t);}
static void* MS MAllocate(U n){return store_cached_tag::CachedTag_Allocate(n);}
static void MS MMask(B*p,int t,B*m){store_cached_tag::CachedTag_BuildMask(p,t,m);}
static void MS MCalc(void*p,B*q,B*m){store_cached_tag::CachedTag_Calculate(p,q,m);}
static void MS MFinalize(B*p,B*m){store_cached_tag::CachedTag_Finalize(p,m);}
struct Capture{U result,flags,calls[10];int found[2],failed;float pos[3];B text[160],cache[0x90],object[0x90];};
extern "C"{
unsigned TextDependencySize(){return sizeof(Capture);}
void* TextDependencyMock(unsigned i){void*p[]={reinterpret_cast<void*>(MLookup),reinterpret_cast<void*>(MResolve),reinterpret_cast<void*>(MSanitize),reinterpret_cast<void*>(MFallback),reinterpret_cast<void*>(MTransform),reinterpret_cast<void*>(MAllocate),reinterpret_cast<void*>(MMask),reinterpret_cast<void*>(MCalc),reinterpret_cast<void*>(MFinalize)};return p[i];}
void TextDependencyRun(void*image,unsigned op,U seed,void*out){
    auto*b=static_cast<B*>(image);Capture c{};U s=seed;auto next=[&](){s=s*1664525u+1013904223u;return s;};
    std::memset(calls,0,sizeof calls);flags=0;absent=(seed&1)!=0;
    unsigned n=next()%159;for(unsigned i=0;i<n;++i)name[i]=i%7?'a'+char(next()%26):'!';name[n]=0;
    std::memset(c.text,0xa5,sizeof c.text);c.found[0]=c.found[1]=99;
    if(op==0){
        int client=seed%2,context=next()%4,filter=next()%3,binding=next()%9;
        Put(modes,0,int(next()%2));Put(modes,24,int(next()%2));for(auto&k:keys)k=int(next()%11);
        if(b){std::memcpy(b+0x1ef91f4,modes,sizeof modes);std::memcpy(b+0xe5d36c,keys,sizeof keys);}
        c.result=b?reinterpret_cast<int(MS*)(int,int,int*,int,int)>(b+0x1a51f0)(client,binding,c.found,context,filter):store_client_text::FindBindingKeys(client,binding,c.found,context,filter);
    }else if(op==1){
        char*dst=(seed&2)?nullptr:reinterpret_cast<char*>(c.text);
        if(b)reinterpret_cast<void(MS*)(int,int,char*)>(b+0x1bf5a0)(0,17,dst);else store_client_text::CopyBindingText(0,17,dst);
    }else if(op<=3){
        std::memset(state,0,sizeof state);std::memset(records,0,sizeof records);Put(state,0x20,static_cast<B*>(records));Put(state,0x28,8);Put(state,0x68,int(seed&2));Put(state,0x2a94,int(next()%9));Put(state,0x2aa4,int(next()%8));
        for(unsigned i=0;i<8;++i)Put(records,i*0x120,int(next()%5));
        int id=next()%6,limit=1+next()%159;B flag=B(next());
        if(b){Put(b,0xe61bf8,static_cast<B*>(state));Put(b,0xe5ca9c,2);}
        if(op==2)c.result=b?reinterpret_cast<U(MS*)(int,int,char*,int,B)>(b+0x1bf600)(0,id,reinterpret_cast<char*>(c.text),limit,flag):store_client_text::FindDisplayText(0,id,reinterpret_cast<char*>(c.text),limit,flag);
        else c.result=b?reinterpret_cast<U(MS*)(int,int,char*,int)>(b+0x1bf710)(0,id,reinterpret_cast<char*>(c.text),limit):store_client_text::FindDisplayTextAlternate(0,id,reinterpret_cast<char*>(c.text),limit);
    }else if(op==4){
        using namespace store_cached_tag;
        std::memset(entity,0,sizeof entity);std::memset(cache,0,sizeof cache);std::memset(transform,0,sizeof transform);
        Put(entity,0x2cc,H(seed&2?1:0));Put(entity,0x330,seed&4?static_cast<B*>(cache):nullptr);Put(entity,0x1f4,5u);indices[5]=seed&8?1:0;
        for(unsigned i=0;i<3;++i){Put(entity,0x40+i*4,float(int(next()%200)-100));Put(transform,0x10+i*4,float(int(next()%200)-100));}
        for(unsigned i=0;i<3;++i){Put(cache,i*0x30,100u);for(unsigned j=0;j<3;++j)Put(cache,i*0x30+4+j*4,float(int(next()%100)-50));}
        Put(gameState,0x4808c,120);CachedTag_Mode=next()%3;U tag=123;switch(next()%3){case 1:tag=456;break;case 2:tag=789;break;}
        int timeout=next()%40;B force=B(seed&16);
        if(b){Put(b,0x25a3308,CachedTag_Mode);Put(b,0x1ef6a0c,CachedTag_First);Put(b,0x1ef6a0a,CachedTag_Second);Put(b,0x1ef6af8,CachedTag_Third);Put(b,0xe53a20,static_cast<B*>(gameState));std::memcpy(b+0x3fcd390,indices,sizeof indices);}
        c.result=b?reinterpret_cast<U(MS*)(B*,U,float*,int,B)>(b+0x106c50)(entity,tag,c.pos,timeout,force):GetCachedTagPosition(entity,tag,c.pos,timeout,force);
        std::memcpy(c.cache,cache,sizeof cache);
    }else{
        using namespace store_cached_tag;
        std::memset(object,0,sizeof object);object[0xe]=B(next()%32);int part=next()%96;int generation=100;
        Put(clientState,0x2cb0,generation);Put(object,0x54,seed&2?generation:99);Put(object,0x58,seed&4?static_cast<B*>(allocation):nullptr);
        if(seed&8)Put(object,0x40+unsigned(part/32)*4,0x80000000u>>(part&31));CachedTag_LastFailureGeneration=17;
        if(b){Put(b,0x27e6a48,0);Put(b,0xe61bf8,static_cast<B*>(clientState));Put(b,0x44a07b8,17);reinterpret_cast<void(MS*)(void*,B*,int)>(b+0xe96b0)(nullptr,object,part);c.failed=Get<int>(b,0x44a07b8);}
        else{EnsureTagGeneration(nullptr,object,part);c.failed=CachedTag_LastFailureGeneration;}
        std::memcpy(c.object,object,sizeof object);
    }
    std::memcpy(c.calls,calls,sizeof calls);c.flags=flags;std::memcpy(out,&c,sizeof c);
}
}
