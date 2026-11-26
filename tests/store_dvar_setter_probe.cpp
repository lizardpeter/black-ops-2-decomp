// Isolated original PE setter differential replay. External allocation,
// release, format and final SetVariant boundaries are compared, not emulated.
#define T6_STORE
#define T6_STORE_DVAR_CORE_REPLAY
#include "../src/universal/dvar_registration_store.cpp"
#include <cassert>
#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <cstdint>
using namespace store_dvar;
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_dvar {
struct Event { U kind,a,b,c; };
struct Trace { U count; Event events[8]; U latched[2]; U current[2]; U reset[2]; U modified; };
static Trace trace{};
static Record d{};
alignas(16) static Value input;
static char pool[4096];
static const char *const strings[]={nullptr,"old","current","reset","current","reset","new","old",""};
static unsigned returnIndex;
static U Hash(const char *s) {
    if(!s) return 0;
    U h=14695981039346656037ull;
    while(*s) { h^=static_cast<unsigned char>(*s++);h*=1099511628211ull; }
    return h;
}
static void Push(U k,U a=0,U b=0,U c=0) {
    assert(trace.count<8);trace.events[trace.count++]={k,a,b,c};
}
extern "C" {
int Store_DvarReaders=0,Store_DvarWriters=0,Store_DvarCount=0;
unsigned char Store_DvarSorted=0;
Record Store_DvarPool[4096]{};
Record *Store_DvarList[4096]{},*Store_DvarBuckets[1024]{};
unsigned char *Store_DvarStringPool=reinterpret_cast<unsigned char *>(pool);
Record *Store_DvarCheats=nullptr;
U32 Store_DvarModifiedFlags=0;
unsigned char Store_DvarCallbacksActive=0;
void Store_DvarSleep(unsigned) { std::abort(); }
int Store_DvarLowercase(int c) {return std::tolower(static_cast<unsigned char>(c));}
U32 Store_DvarIntern(const char *s,unsigned user,unsigned len) {
    assert(user==0 && len==std::strlen(s)+1 && len<=24);
    Push(1,Hash(s),len,user);
    if(returnIndex)std::memcpy(pool+4+24*returnIndex,s,len);
    return returnIndex;
}
[[noreturn]] void Store_DvarCapacityError(int,const char *,const char *,int) {std::abort();}
void Store_DvarRelease(const char *s) {Push(2,reinterpret_cast<U>(s));}
const char *Store_DvarFormat(Record *,const Value *) {std::abort();}
Value *Store_DvarParse(Value *,int,const Limits *,const char *) {std::abort();}
Value *Store_DvarConstrain(Value *,int,const Value *,const Value *,const Limits *) {std::abort();}
void Store_DvarSetVariant(Record *p,const Value *v,int source) {
    assert(p==&d);
    Push(4,p->type==2?v->words[0]:Hash(v->string),source,p->type);
}
void Store_DvarSetLatched(Record *,const Value *) {std::abort();}
int Store_DvarFormatBuffer(char *s,unsigned n,const char *fmt,double value) {
    assert(n==32 && !std::strcmp(fmt,"%g"));
    U bits; std::memcpy(&bits,&value,8);Push(3,n,Hash(fmt),bits);
    return std::snprintf(s,n,fmt,value);
}
}
static U32 MS MIntern(const char *s,unsigned u,unsigned n){return Store_DvarIntern(s,u,n);}
static void MS MRelease(const char *s){Store_DvarRelease(s);}
static void MS MVariant(Record *p,const Value *v,int src){Store_DvarSetVariant(p,v,src);}
// The original variadic caller mirrors XMM3 in R9. A fixed double
// fourth argument captures the value using the same Microsoft x64 ABI.
static int MS MFormat(char *s,unsigned n,const char *fmt,double v){return Store_DvarFormatBuffer(s,n,fmt,v);}
extern "C" {
void *CoreSetterMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MIntern),reinterpret_cast<void *>(MRelease),
               reinterpret_cast<void *>(MVariant),reinterpret_cast<void *>(MFormat)};
    assert(i<4);return m[i];
}
void *CoreSetterPool(){return pool;}
unsigned CoreSetterSize(){return sizeof(Trace);}
void CoreLatchRun(void *base,int type,unsigned cur,unsigned lat,unsigned reset,unsigned next,unsigned index,void *out) {
    trace={};d={};input={};returnIndex=index;
    d.type=type;d.modified=0xa5;
    d.current=Value{{0x11112222,0x33334444,0x55556666,0x77778888}};
    d.latched=Value{{0xabcdef01,0xabcdef02,0xabcdef03,0xabcdef04}};
    d.reset=Value{{0x12345671,0x12345672,0x12345673,0x12345674}};
    input=Value{{0x10203040,0x50607080,0x90a0b0c0,0xd0e0f000}};
    if(type==8) {
        d.current.string=strings[cur];d.latched.string=strings[lat];d.reset.string=strings[reset];
        input.string=strings[next];
    }
    if(base)reinterpret_cast<void(MS *)(Record *,const Value *)>(static_cast<char *>(base)+0x46b150)(&d,&input);
    else SetLatched(&d,&input);
    std::memcpy(trace.latched,&d.latched,16);std::memcpy(trace.current,&d.current,16);
    std::memcpy(trace.reset,&d.reset,16);trace.modified=d.modified;
    std::memcpy(out,&trace,sizeof(trace));
}
void CoreFloatRun(void *base,int type,unsigned named,U32 bits,int source,void *out) {
    trace={};d={};d.type=type;d.name=named==0?nullptr:named==1?"":"test";
    float value;std::memcpy(&value,&bits,4);
    Record *p=named==3?nullptr:&d;
    if(base)reinterpret_cast<void(MS *)(Record *,float,int)>(static_cast<char *>(base)+0x46cef0)(p,value,source);
    else SetFloat(p,value,source);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
