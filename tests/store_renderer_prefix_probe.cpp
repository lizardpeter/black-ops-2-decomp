// Original-PE prefix replay. No renderer or game is launched.
#define T6_STORE
#define T6_STORE_RENDERER_DVAR_REPLAY
#include "../src/gfx_d3d/r_dvars_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_renderer_dvar {
struct Dvar { U tag; };
extern "C" {
const Dvar *store_r_fontResolution=nullptr;
const Dvar *store_r_resolution=nullptr;
const Dvar *store_r_ssao=nullptr;
const Dvar *store_r_ssaoScale=nullptr;
const Dvar *store_r_ssaoRadius=nullptr;
const Dvar *store_r_ssaoBias=nullptr;
const Dvar *store_r_ssaoIntensity=nullptr;
}
namespace {
struct Event { U kind,name,type,flags,value,min,max; };
struct Trace { U count; Event events[16]; };
Trace trace{};
unsigned mask;
Dvar slots[8]{};
unsigned used;
U Hash(const char *s) {
    U h=14695981039346656037ull;
    while (*s) { h^=static_cast<unsigned char>(*s++); h*=1099511628211ull; }
    return h;
}
void Push(U k,U n=0,U t=0,U f=0,U v=0,U lo=0,U hi=0) {
    assert(trace.count<16);
    trace.events[trace.count++]={k,n,t,f,v,lo,hi};
}
Dvar *Slot(const char *n) {
    U h=Hash(n);
    for (unsigned i=0;i<used;++i) if(slots[i].tag==h) return &slots[i];
    assert(used<8); slots[used].tag=h; return &slots[used++];
}
U32 Bits(float f) { U32 v; std::memcpy(&v,&f,4); return v; }
void Empty(const char *s) { assert(s && !*s); }
}
extern "C" {
void Store_Renderer_Init() { Push(0); }
const Dvar *Store_Renderer_Find(const char *n) {
    unsigned bit;
    if (!std::strcmp(n,"r_fontResolution")) bit=1;
    else { assert(!std::strcmp(n,"r_resolution")); bit=2; }
    const bool exists=(mask&bit)!=0;
    Push(1,Hash(n),0,0,exists);
    return exists?Slot(n):nullptr;
}
const Dvar *Store_Renderer_New(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) {
    Empty(s); Push(2,Hash(n),t,f,v->words[0],d->words[0],d->words[1]); return Slot(n);
}
void Store_Renderer_Reregister(const Dvar *old,const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) {
    Empty(s); assert(old==Slot(n)); Push(3,Hash(n),t,f,v->words[0],d->words[0],d->words[1]);
}
const Dvar *Store_Renderer_Bool(const char *n,bool v,U32 f,const char *s) {
    Empty(s); Push(4,Hash(n),1,f,v); return Slot(n);
}
const Dvar *Store_Renderer_Float(const char *n,float v,float lo,float hi,U32 f,const char *s) {
    Empty(s); Push(5,Hash(n),2,f,Bits(v),Bits(lo),Bits(hi)); return Slot(n);
}
}
static void MS MInit() { Store_Renderer_Init(); }
static const Dvar *MS MFind(const char *n) { return Store_Renderer_Find(n); }
static const Dvar *MS MNew(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { return Store_Renderer_New(n,t,f,v,d,s); }
static void MS MReg(const Dvar *p,const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { Store_Renderer_Reregister(p,n,t,f,v,d,s); }
static const Dvar *MS MBool(const char *n,bool v,U32 f,const char *s) { return Store_Renderer_Bool(n,v,f,s); }
static const Dvar *MS MFloat(const char *n,float v,float lo,float hi,U32 f,const char *s) { return Store_Renderer_Float(n,v,lo,hi,f,s); }
extern "C" {
void *RendererPrefixMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MInit),reinterpret_cast<void *>(MFind),reinterpret_cast<void *>(MNew),
               reinterpret_cast<void *>(MReg),reinterpret_cast<void *>(MBool),reinterpret_cast<void *>(MFloat)};
    assert(i<6); return m[i];
}
unsigned RendererPrefixSize() { return sizeof(Trace); }
void RendererPrefixRun(void *image,unsigned state,void *out) {
    trace={}; used=0; mask=state;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererEntryPrefix();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererPrefixGlobals(U *out) {
    const Dvar *slots_[]={store_r_fontResolution,store_r_resolution,store_r_ssao,store_r_ssaoScale,
                          store_r_ssaoRadius,store_r_ssaoBias,store_r_ssaoIntensity};
    for(unsigned i=0;i<7;++i) out[i]=reinterpret_cast<U>(slots_[i]);
}
}
} // namespace store_renderer_dvar
