#define T6_STORE
#define T6_STORE_CG_FOV_MODE_REPLAY
#include "../src/cgame_mp/cg_fov_mode_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_cg_fov {
struct Dvar { unsigned char data[104]; };
static Dvar slots[8]{};
struct Event { U kind,target,a,b; };
struct Trace { U count; Event events[8]; };
static Trace trace{};
static U Bits(float x) { std::uint32_t b;std::memcpy(&b,&x,4);return b; }
static U Id(const Dvar *d) { return d?U(d-slots)+1:0; }
static void Push(U k,U t,U a=0,U b=0) {
    assert(trace.count<8);trace.events[trace.count++]={k,t,a,b};
}
extern "C" {
const Dvar *FovMode_DualPlay,*FovMode_Height,*FovMode_Width,*FovMode_Offset,*FovMode_PointerHeight,
           *FovMode_PointerWidth,*FovMode_PointerPivot,*FovMode_Scale;
int FovMode_Atoi(const char *s) { int v=std::atoi(s);Push(1,0,U(v));return v;}
void FovMode_SetFloat(const Dvar *d,float x,int source){Push(2,Id(d),Bits(x),U(source));}
void FovMode_SetVec2(const Dvar *d,float x,float y){Push(3,Id(d),Bits(x),Bits(y));}
}
static int MS MAtoi(const char *s){return FovMode_Atoi(s);}
static void MS MFloat(const Dvar *d,float x,int src){FovMode_SetFloat(d,x,src);}
static void MS MVec2(const Dvar *d,float x,float y){FovMode_SetVec2(d,x,y);}
extern "C" {
void *FovModeMock(unsigned i) {
    void *m[]={reinterpret_cast<void *>(MAtoi),reinterpret_cast<void *>(MFloat),reinterpret_cast<void *>(MVec2)};
    assert(i<3);return m[i];
}
unsigned FovModeSize(){return sizeof(Trace);}
void FovModeRun(void *base,unsigned alternate,unsigned dual,unsigned mask,unsigned stringCase,void *out) {
    trace={};
    const char *strings[]={"0","1","-1","15","abc",""};
    const char *s=strings[stringCase];
    int type=dual==1?1:8;
    std::memcpy(slots[0].data+0x18,&type,4);
    std::memcpy(slots[0].data+0x20,&s,sizeof(s));
    FovMode_DualPlay=dual?&slots[0]:nullptr;
    const Dvar **globals[]={&FovMode_Height,&FovMode_Width,&FovMode_Offset,&FovMode_PointerHeight,
                          &FovMode_PointerWidth,&FovMode_PointerPivot,&FovMode_Scale};
    const unsigned rvas[]={0xe4f8b8,0xe4f8c8,0xe4f8a0,0xe4f8b0,0xe4f8a8,0xe4f8d0,0xd78658};
    for(unsigned i=0;i<7;++i) {
        *globals[i]=(mask&(1u<<i))?&slots[i+1]:nullptr;
        if(base)std::memcpy(static_cast<char *>(base)+rvas[i],globals[i],sizeof(void *));
    }
    if(base) {
        std::memcpy(static_cast<char *>(base)+0x27e6e50,&FovMode_DualPlay,sizeof(void *));
        reinterpret_cast<void(MS *)(bool)>(static_cast<char *>(base)+0x101410)(alternate!=0);
    } else SetHudAndFovMode(alternate!=0);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
