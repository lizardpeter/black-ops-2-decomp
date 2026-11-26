// Original-PE prefix replay. No renderer or game is launched.
#define T6_STORE
#define T6_STORE_RENDERER_DVAR_REPLAY
#include "../src/gfx_d3d/r_dvars_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_renderer_dvar {
struct Dvar { alignas(8) unsigned char bytes[104]; U tag; };
extern "C" {
U32 Store_Renderer_HardwareCount=0;
const Dvar *store_r_smp_worker_threads=nullptr;
const Dvar *store_r_smp_worker_thread[8]{};
const Dvar *store_r_txaa=nullptr;
const Dvar *store_r_txaaSupported=nullptr;
const Dvar *store_r_txaaDepth1=nullptr;
const Dvar *store_r_txaaDepth2=nullptr;
const Dvar *store_r_txaaMotion1=nullptr;
const Dvar *store_r_txaaMotion2=nullptr;
const Dvar *store_vid_xpos=nullptr;
const Dvar *store_vid_ypos=nullptr;
const Dvar *store_r_fullscreen=nullptr;
const Dvar *store_r_previousFullscreen=nullptr;
const Dvar *store_r_convergence=nullptr;
const Dvar *store_r_ignorehwgamma=nullptr;
const Dvar *store_r_texFilterAnisoMax=nullptr;
const Dvar *store_r_texFilterAnisoMin=nullptr;
const Dvar *store_r_texFilterMipMode=nullptr;
const char *const Store_Renderer_ClipCodec[]={nullptr};
const char *const Store_Renderer_ClipSize[]={nullptr};
const char *const Store_Renderer_MipMode[]={nullptr};
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
struct Trace { U count; Event events[64]; };
Trace trace{};
unsigned mask;
void *mapped_image;
bool worker_mode=false;
unsigned worker_type;
int worker_value;
Dvar slots[64]{};
unsigned used;
U Hash(const char *s) {
    U h=14695981039346656037ull;
    while (*s) { h^=static_cast<unsigned char>(*s++); h*=1099511628211ull; }
    return h;
}
void Push(U k,U n=0,U t=0,U f=0,U v=0,U lo=0,U hi=0) {
    assert(trace.count<64);
    trace.events[trace.count++]={k,n,t,f,v,lo,hi};
}
Dvar *Slot(const char *n) {
    U h=Hash(n);
    for (unsigned i=0;i<used;++i) if(slots[i].tag==h) return &slots[i];
    assert(used<64); slots[used].tag=h; return &slots[used++];
}
template<class T> void Write(Dvar *d,unsigned off,T v) { std::memcpy(d->bytes+off,&v,sizeof(v)); }
Dvar *WorkerSlot(const char *n) {
    auto *d=Slot(n);
    if (!std::strcmp(n,"r_smp_worker_threads")) {
        Write(d,0x18,int(worker_type));
        switch(worker_type) {
        case 1: Write(d,0x20,static_cast<unsigned char>(worker_value!=0)); break;
        case 2: Write(d,0x20,float(worker_value)+0.25f); break;
        case 6: case 7: Write(d,0x20,worker_value); break;
        default: {
            static const char *const nums[]={"0","1","2","3","4","5","6","7","8"};
            Write(d,0x20,worker_value<0?"-1":nums[worker_value]); break;
        }
        }
    }
    return d;
}
U32 Bits(float f) { U32 v; std::memcpy(&v,&f,4); return v; }
void Empty(const char *s) { assert(s && !*s); }
}
extern "C" {
void Store_Renderer_Init() { Push(0); }
const Dvar *Store_Renderer_Find(const char *n) {
    if(worker_mode) {
        assert(!std::strcmp(n,"r_smp_worker_threads"));
        const bool exists=(mask&1)!=0;
        Push(1,Hash(n),0,0,exists);
        return exists?WorkerSlot(n):nullptr;
    }
    const char *names[]={"r_fontResolution","r_resolution","r_txaaMode","r_clipFPS","vid_xpos","vid_ypos","r_fullscreen","r_previousFullscreen","r_texFilterAnisoMax","r_texFilterAnisoMin"};
    unsigned index=0; while(index<10 && std::strcmp(n,names[index])) ++index;
    assert(index<10); const unsigned bit=1u<<index;
    const bool exists=(mask&bit)!=0;
    Push(1,Hash(n),0,0,exists);
    return exists?Slot(n):nullptr;
}
const Dvar *Store_Renderer_New(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) {
    Empty(s); Push(2,Hash(n),t,f,v->words[0],d->words[0],d->words[1]); return worker_mode?WorkerSlot(n):Slot(n);
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
const Dvar *Store_Renderer_Enum(const char *n,const char *const *o,int v,U32 f) {
    const char *names[]={"r_clipCodec","r_clipSize","r_texFilterMipMode"};
    const char *const *tables[]={Store_Renderer_ClipCodec,Store_Renderer_ClipSize,Store_Renderer_MipMode};
    const unsigned rvas[]={0xd39f30,0xd3a1e0,0xd3a138};
    unsigned i=0; while(i<3 && std::strcmp(n,names[i])) ++i;
    assert(i<3);
    if(mapped_image) assert(reinterpret_cast<const void *>(o)==static_cast<unsigned char *>(mapped_image)+rvas[i]);
    else assert(o==tables[i]);
    Push(6,Hash(n),7,f,U32(v),rvas[i]); return Slot(n);
}
}
static const Dvar *MS MEnum(const char *n,const char *const *o,int v,U32 f) { return Store_Renderer_Enum(n,o,v,f); }
static int MS MAtoi(const char *n) { return std::atoi(n); }
static void MS MInit() { Store_Renderer_Init(); }
static const Dvar *MS MFind(const char *n) { return Store_Renderer_Find(n); }
static const Dvar *MS MNew(const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { return Store_Renderer_New(n,t,f,v,d,s); }
static void MS MReg(const Dvar *p,const char *n,int t,U32 f,const Value *v,const Domain *d,const char *s) { Store_Renderer_Reregister(p,n,t,f,v,d,s); }
static const Dvar *MS MBool(const char *n,bool v,U32 f,const char *s) { return Store_Renderer_Bool(n,v,f,s); }
static const Dvar *MS MFloat(const char *n,float v,float lo,float hi,U32 f,const char *s) { return Store_Renderer_Float(n,v,lo,hi,f,s); }
extern "C" {
void *RendererPrefixMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MInit),reinterpret_cast<void *>(MFind),reinterpret_cast<void *>(MNew),
               reinterpret_cast<void *>(MReg),reinterpret_cast<void *>(MBool),reinterpret_cast<void *>(MFloat),reinterpret_cast<void *>(MEnum),reinterpret_cast<void *>(MAtoi)};
    assert(i<8); return m[i];
}
unsigned RendererPrefixSize() { return sizeof(Trace); }
void RendererPrefixRun(void *image,unsigned state,void *out) {
    trace={}; used=0; mask=state; mapped_image=image; worker_mode=false;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererEntryPrefix();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererWorkerRun(void *image,unsigned exists,unsigned hardware,unsigned type,int value,void *out) {
    trace={}; used=0; mask=exists; mapped_image=image; worker_mode=true;
    worker_type=type; worker_value=value; Store_Renderer_HardwareCount=hardware;
    store_r_smp_worker_threads=nullptr;
    for(auto &d:store_r_smp_worker_thread) d=nullptr;
    if(image) {
        using Entry=void(MS *)();
        reinterpret_cast<Entry>(static_cast<unsigned char *>(image)+0x70e230)();
    } else RegisterRendererWorkerFragment();
    std::memcpy(out,&trace,sizeof(trace));
}
void RendererWorkerGlobals(U *out) {
    out[0]=reinterpret_cast<U>(store_r_smp_worker_threads);
    for(unsigned i=0;i<8;++i) out[i+1]=reinterpret_cast<U>(store_r_smp_worker_thread[i]);
}
void RendererPrefixGlobals(U *out) {
    const Dvar *slots_[]={store_r_fontResolution,store_r_resolution,store_r_ssao,store_r_ssaoScale,
                          store_r_ssaoRadius,store_r_ssaoBias,store_r_ssaoIntensity,
                          store_r_txaa,store_r_txaaSupported,store_r_txaaDepth1,store_r_txaaDepth2,store_r_txaaMotion1,store_r_txaaMotion2,store_vid_xpos,store_vid_ypos,store_r_fullscreen,store_r_previousFullscreen,store_r_convergence,store_r_ignorehwgamma,store_r_texFilterAnisoMax,store_r_texFilterAnisoMin,store_r_texFilterMipMode};
    for(unsigned i=0;i<22;++i) out[i]=reinterpret_cast<U>(slots_[i]);
}
}
} // namespace store_renderer_dvar
