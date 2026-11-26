// Compare the complete common registration body's observable calls and result stores.
#define T6_STORE
#define T6_STORE_COMMON_DVAR_REPLAY
#include "../src/qcommon/common_register_dvars_store.cpp"
#include <cassert>
#include <array>
#include <cstddef>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_common_dvar {
struct Dvar { alignas(8) unsigned char bytes[104]; };
extern "C" {
const Dvar *store_comDvar_141f04878=nullptr;
const Dvar *store_comDvar_141f04908=nullptr;
const Dvar *store_comDvar_141f03820=nullptr;
const Dvar *store_comDvar_141f04910=nullptr;
const Dvar *store_comDvar_141f04998=nullptr;
const Dvar *store_comDvar_141f04850=nullptr;
const Dvar *store_comDvar_141f049d8=nullptr;
const Dvar *store_comDvar_141f037a8=nullptr;
const Dvar *store_comDvar_141f037c8=nullptr;
const Dvar *store_comDvar_141f04858=nullptr;
const Dvar *store_comDvar_141f04848=nullptr;
const Dvar *store_comDvar_141f02f60=nullptr;
const Dvar *store_comDvar_141f04870=nullptr;
const Dvar *store_comDvar_141f02f70=nullptr;
const Dvar *store_comDvar_141f04840=nullptr;
const Dvar *store_comDvar_141f02f50=nullptr;
const Dvar *store_comDvar_141f037c0=nullptr;
const Dvar *store_comDvar_141f04950=nullptr;
const Dvar *store_comDvar_141f037a0=nullptr;
const Dvar *store_comDvar_141f03798=nullptr;
const Dvar *store_comDvar_141f04920=nullptr;
const Dvar *store_comDvar_141f037e8=nullptr;
const Dvar *store_comDvar_141f037d0=nullptr;
const Dvar *store_comDvar_141f02f68=nullptr;
const Dvar *store_comDvar_141f04868=nullptr;
const Dvar *store_comDvar_141f048e8=nullptr;
const Dvar *store_comDvar_141f04930=nullptr;
const Dvar *store_comDvar_141f04888=nullptr;
const Dvar *store_comDvar_141f049e8=nullptr;
const Dvar *store_comDvar_141f04990=nullptr;
const Dvar *store_comDvar_141f048f0=nullptr;
const Dvar *store_comDvar_141f04938=nullptr;
const Dvar *store_comDvar_141f04948=nullptr;
const Dvar *store_comDvar_141f04958=nullptr;
const Dvar *store_comDvar_141f02f48=nullptr;
const Dvar *store_comDvar_141f037d8=nullptr;
const Dvar *store_comDvar_142126558=nullptr;
const Dvar *store_comDvar_142155d00=nullptr;
const Dvar *store_comDvar_141f037e0=nullptr;
const Dvar *store_comDvar_141f02ef0=nullptr;
const Dvar *store_comDvar_141f037b8=nullptr;
const Dvar *store_comDvar_141f03830=nullptr;
const Dvar *store_comDvar_141f03818=nullptr;
const Dvar *store_comDvar_141f04900=nullptr;
const Dvar *store_comDvar_141f02f58=nullptr;
const Dvar *store_comDvar_141f03808=nullptr;
const Dvar *store_comDvar_141f049a0=nullptr;
const Dvar *store_comDvar_141f02ef8=nullptr;
const Dvar *store_comDvar_141f048e0=nullptr;
const Dvar *store_comDvar_141f048d8=nullptr;
const Dvar *store_comDvar_141f048c8=nullptr;
const Dvar *store_comDvar_141f048c0=nullptr;
const Dvar *store_comDvar_141f04880=nullptr;
const Dvar *store_comDvar_141f049b8=nullptr;
const Dvar *store_comDvar_141f03810=nullptr;
const Dvar *store_comDvar_141f03838=nullptr;
const Dvar *store_comDvar_141f037f0=nullptr;
const Dvar *store_comDvar_141f02f00=nullptr;
const Dvar *store_comDvar_141f037b0=nullptr;
const Dvar *store_comDvar_141f037f8=nullptr;
const Dvar *store_comDvar_141f02f08=nullptr;
const Dvar *store_comDvar_141f049a8=nullptr;
const Dvar *store_comDvar_141f049b0=nullptr;
const Dvar *store_comDvar_141f049d0=nullptr;
const Dvar *store_comDvar_141f049c0=nullptr;
const Dvar *store_comDvar_141f049c8=nullptr;
const Dvar *store_comDvar_141f048d0=nullptr;
const Dvar *store_comDvar_141f049f0=nullptr;
const Dvar *store_comDvar_141de1438=nullptr;
const Dvar *store_comDvar_141f04988=nullptr;
const Dvar *Store_Common_War[5]{};
const char *const Store_Common_DedicatedOptions[]={"listen server",nullptr};
}
namespace {
struct Event { U kind,name,type,flags,value,min,max,extra; };
struct Trace { U count; Event events[180]; };
static_assert(sizeof(Event)==64);
Trace trace{};
unsigned seed;
void *image;
struct Slot { U name; Dvar d; };
Slot slots[128]{};
unsigned used;
U Hash(const char *s) {
    U h=14695981039346656037ull;
    while (*s) { h^=static_cast<unsigned char>(*s++); h*=1099511628211ull; }
    return h;
}
void EventPush(U k,U n=0,U t=0,U f=0,U v=0,U lo=0,U hi=0,U e=0) {
    assert(trace.count<180);
    trace.events[trace.count++]={k,n,t,f,v,lo,hi,e};
}
Dvar *GetSlot(const char *n) {
    U h=Hash(n);
    for (unsigned i=0;i<used;++i) if (slots[i].name==h) return &slots[i].d;
    assert(used<128); slots[used].name=h;
    return &slots[used++].d;
}
U SlotName(const Dvar *d) {
    for (unsigned i=0;i<used;++i) if (&slots[i].d==d) return slots[i].name;
    std::abort();
}
template<class T> void Write(Dvar *d,unsigned off,T v) { std::memcpy(d->bytes+off,&v,sizeof(v)); }
U32 Bits(float f) { U32 v; std::memcpy(&v,&f,4); return v; }
void Empty(const char *s) { assert(s && !*s); }
bool DedicatedNonzero() { return (seed%10)&1; }
}
extern "C" {
const Dvar *Store_Common_Find(const char *n) {
    U h=Hash(n);
    bool exists=((h^(h>>23)^U(seed*0x9e3779b9u))&1)!=0;
    EventPush(1,h,0,0,exists);
    auto *d=GetSlot(n);
    return exists?d:nullptr;
}
const Dvar *Store_Common_New(const char *n,int t,U32 f,const Value *v,const Domain *l,const char *desc) {
    Empty(desc);
    EventPush(2,Hash(n),t,f,t==1?v->words[0]&255u:v->words[0],l->words[0],l->words[1]);
    return GetSlot(n);
}
void Store_Common_Reregister(const Dvar *d,const char *n,int t,U32 f,const Value *v,const Domain *l,const char *desc) {
    Empty(desc); assert(d==GetSlot(n));
    EventPush(3,Hash(n),t,f,t==1?v->words[0]&255u:v->words[0],l->words[0],l->words[1]);
}
const Dvar *Store_Common_Bool(const char *n,bool v,U32 f,const char *desc) {
    Empty(desc); EventPush(4,Hash(n),1,f,v); return GetSlot(n);
}
const Dvar *Store_Common_Float(const char *n,float v,float lo,float hi,U32 f,const char *desc) {
    Empty(desc); EventPush(5,Hash(n),2,f,Bits(v),Bits(lo),Bits(hi)); return GetSlot(n);
}
const Dvar *Store_Common_String(const char *n,const char *v,U32 f) {
    Empty(v); EventPush(6,Hash(n),8,f,Hash(v)); return GetSlot(n);
}
const Dvar *Store_Common_Enum(const char *n,const char *const *options,int v,U32 f) {
    assert(std::strcmp(n,"dedicated")==0);
    if (image) assert(reinterpret_cast<const void *>(options)==static_cast<unsigned char *>(image)+0xa53d90);
    else assert(options==Store_Common_DedicatedOptions);
    EventPush(7,Hash(n),7,f,U32(v));
    if (seed%10==8) return nullptr;
    Dvar *d=GetSlot(n);
    switch (seed%10) {
    case 0: case 1: Write(d,0x18,7); Write(d,0x20,int(seed%10)); break;
    case 2: case 3: Write(d,0x18,1); Write(d,0x20,int(seed%10==3)); break;
    case 4: case 5: Write(d,0x18,2); Write(d,0x20,seed%10==4?0.5f:-1.5f); break;
    case 6: case 7: Write(d,0x18,8); Write(d,0x20,seed%10==6?"0":"7"); break;
    case 9: Write(d,0x18,6); Write(d,0x20,-1); break;
    }
    return d;
}
void Store_Common_SetIntFromSource(const Dvar *d,int v,int source) {
    EventPush(8,SlotName(d),0,0,U32(v),0,0,U32(source));
}
void Store_Common_Sub_1402983A0() { EventPush(9); }
bool Store_Common_ExitAfterConnectPaths() { bool b=(seed&2)!=0; EventPush(10,0,0,0,b); return b; }
}
static const Dvar *MS MFind(const char *n) { return Store_Common_Find(n); }
static const Dvar *MS MNew(const char *n,int t,U32 f,const Value *v,const Domain *l,const char *d) { return Store_Common_New(n,t,f,v,l,d); }
static void MS MReg(const Dvar *p,const char *n,int t,U32 f,const Value *v,const Domain *l,const char *d) { Store_Common_Reregister(p,n,t,f,v,l,d); }
static const Dvar *MS MBool(const char *n,bool v,U32 f,const char *d) { return Store_Common_Bool(n,v,f,d); }
static const Dvar *MS MFloat(const char *n,float v,float lo,float hi,U32 f,const char *d) { return Store_Common_Float(n,v,lo,hi,f,d); }
static const Dvar *MS MString(const char *n,const char *v,U32 f) { return Store_Common_String(n,v,f); }
static const Dvar *MS MEnum(const char *n,const char *const *o,int v,U32 f) { return Store_Common_Enum(n,o,v,f); }
static void MS MSet(const Dvar *p,int v,int s) { Store_Common_SetIntFromSource(p,v,s); }
static void MS MSub() { Store_Common_Sub_1402983A0(); }
static bool MS MExit() { return Store_Common_ExitAfterConnectPaths(); }
static int MS MAtoi(const char *s) { return std::atoi(s); }
extern "C" {
void *CommonMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MFind),reinterpret_cast<void *>(MNew),reinterpret_cast<void *>(MReg),
        reinterpret_cast<void *>(MBool),reinterpret_cast<void *>(MFloat),reinterpret_cast<void *>(MString),
        reinterpret_cast<void *>(MEnum),reinterpret_cast<void *>(MSet),reinterpret_cast<void *>(MSub),
        reinterpret_cast<void *>(MExit),reinterpret_cast<void *>(MAtoi)};
    assert(i<sizeof(m)/sizeof(m[0])); return m[i];
}
unsigned CommonSize() { return sizeof(Trace); }
void CommonRun(void *base,unsigned s,void *out) {
    image=base; seed=s; used=0;
    std::memset(slots,0,sizeof(slots)); std::memset(&trace,0,sizeof(trace));
    if (base) {
        using Entry=void (MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(base)+0x39e2b0)();
    } else RegisterCommonDvars();
    assert(trace.count==145u+unsigned(DedicatedNonzero()));
    std::memcpy(out,&trace,sizeof(trace));
}
unsigned CommonGlobals(U *out) {
    const Dvar *globals[]={store_comDvar_141f04878,
        store_comDvar_141f04908,
        store_comDvar_141f03820,
        store_comDvar_141f04910,
        store_comDvar_141f04998,
        store_comDvar_141f04850,
        store_comDvar_141f049d8,
        store_comDvar_141f037a8,
        store_comDvar_141f037c8,
        store_comDvar_141f04858,
        store_comDvar_141f04848,
        store_comDvar_141f02f60,
        store_comDvar_141f04870,
        store_comDvar_141f02f70,
        store_comDvar_141f04840,
        store_comDvar_141f02f50,
        store_comDvar_141f037c0,
        store_comDvar_141f04950,
        store_comDvar_141f037a0,
        store_comDvar_141f03798,
        store_comDvar_141f04920,
        store_comDvar_141f037e8,
        store_comDvar_141f037d0,
        store_comDvar_141f02f68,
        store_comDvar_141f04868,
        store_comDvar_141f048e8,
        store_comDvar_141f04930,
        store_comDvar_141f04888,
        store_comDvar_141f049e8,
        store_comDvar_141f04990,
        store_comDvar_141f048f0,
        store_comDvar_141f04938,
        store_comDvar_141f04948,
        store_comDvar_141f04958,
        store_comDvar_141f02f48,
        store_comDvar_141f037d8,
        store_comDvar_142126558,
        store_comDvar_142155d00,
        store_comDvar_141f037e0,
        store_comDvar_141f02ef0,
        store_comDvar_141f037b8,
        store_comDvar_141f03830,
        store_comDvar_141f03818,
        store_comDvar_141f04900,
        store_comDvar_141f02f58,
        store_comDvar_141f03808,
        store_comDvar_141f049a0,
        store_comDvar_141f02ef8,
        store_comDvar_141f048e0,
        store_comDvar_141f048d8,
        store_comDvar_141f048c8,
        store_comDvar_141f048c0,
        store_comDvar_141f04880,
        store_comDvar_141f049b8,
        store_comDvar_141f03810,
        store_comDvar_141f03838,
        store_comDvar_141f037f0,
        store_comDvar_141f02f00,
        store_comDvar_141f037b0,
        store_comDvar_141f037f8,
        store_comDvar_141f02f08,
        store_comDvar_141f049a8,
        store_comDvar_141f049b0,
        store_comDvar_141f049d0,
        store_comDvar_141f049c0,
        store_comDvar_141f049c8,
        store_comDvar_141f048d0,
        store_comDvar_141f049f0,
        store_comDvar_141de1438,
        store_comDvar_141f04988,
        Store_Common_War[0],Store_Common_War[1],Store_Common_War[2],Store_Common_War[3],Store_Common_War[4]};
    for (unsigned i=0;i<sizeof(globals)/sizeof(globals[0]);++i) out[i]=reinterpret_cast<U>(globals[i]);
    return sizeof(globals)/sizeof(globals[0]);
}
}
} // namespace store_common_dvar
