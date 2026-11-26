#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_weapon_fov_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_weapon_fov {
struct Dvar{Byte bytes[104];};
static Byte state[0x88200],ps[0xc10],def[0x280],props[0xc00],attachments[3][0x50],sentinel[0x50];
static const Byte* table[64];
static Dvar dvar;
static const char* values[]={"0","1","-1","abc"};
struct Trace {U result,progress,time,events[6],arguments[6];};
static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static U Bits(float f){U32 x;std::memcpy(&x,&f,4);return x;}
extern "C" {
Byte *WeaponFov_State;
const Byte *WeaponFov_Defs[256],*WeaponFov_Sentinel;
const Dvar *WeaponFov_VariableZoom;
__attribute__((visibility("hidden"))) unsigned wf_calls[6],wf_args[6],wf_flags;
__attribute__((visibility("hidden"))) float wf_transition;
__attribute__((visibility("hidden"))) const void *wf_pointer;
int WeaponFov_Atoi(const char*s){++wf_calls[0];return s==values[1]?1:s==values[2]?-1:0;}
bool WeaponFov_Sub_14065B8C0(U32 w){++wf_calls[1];wf_args[1]=w;return (wf_flags&1)!=0;}
const void *WeaponFov_Sub_14065C600(U32 w){++wf_calls[2];wf_args[2]=w;return wf_pointer;}
int WeaponFov_Sub_140646920(const Byte*p){assert(p==ps);++wf_calls[3];return (wf_flags&2)!=0;}
float WeaponFov_TransitionA(U32 w){++wf_calls[4];wf_args[4]=w;return wf_transition;}
float WeaponFov_TransitionB(U32 w){++wf_calls[5];wf_args[5]=w;return wf_transition;}
}
static int MS MAtoi(const char*s){return WeaponFov_Atoi(s);}
// Original LTCG call sites retain r9/r10/r11 and XMM4/5 across calls that
// standard x64 ABI would allow to clobber them. These mocks preserve the
// caller's live registers; each writes only its result and a trace cell.
extern "C" __attribute__((ms_abi,naked)) bool WfFlag(U32){
    __asm__("incl wf_calls+4(%rip)\n\tmovl %ecx,wf_args+4(%rip)\n\tmovl wf_flags(%rip),%eax\n\tandl $1,%eax\n\tret");
}
extern "C" __attribute__((ms_abi,naked)) const void* WfPointer(U32){
    __asm__("incl wf_calls+8(%rip)\n\tmovl %ecx,wf_args+8(%rip)\n\tmovq wf_pointer(%rip),%rax\n\tret");
}
extern "C" __attribute__((ms_abi,naked)) int WfState(const Byte*){
    __asm__("incl wf_calls+12(%rip)\n\tmovl wf_flags(%rip),%eax\n\tandl $2,%eax\n\tret");
}
extern "C" __attribute__((ms_abi,naked)) float WfA(U32){
    __asm__("incl wf_calls+16(%rip)\n\tmovl %ecx,wf_args+16(%rip)\n\tmovss wf_transition(%rip),%xmm0\n\tret");
}
extern "C" __attribute__((ms_abi,naked)) float WfB(U32){
    __asm__("incl wf_calls+20(%rip)\n\tmovl %ecx,wf_args+20(%rip)\n\tmovss wf_transition(%rip),%xmm0\n\tret");
}
extern "C" {
void* WeaponFovMock(unsigned i){
    void *m[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(WfFlag),reinterpret_cast<void*>(WfPointer),
               reinterpret_cast<void*>(WfState),reinterpret_cast<void*>(WfA),reinterpret_cast<void*>(WfB)};
    assert(i<6);return m[i];
}
unsigned WeaponFovTraceSize(){return sizeof(Trace);}
void WeaponFovRun(void*image,U32 key,U32 config,U32 fraction,U32 initial,U32 transition,int now,int last,void*out){
    trace={};std::memset(wf_calls,0,sizeof(wf_calls));std::memset(wf_args,0,sizeof(wf_args));
    wf_flags=(config>>3)&3;wf_pointer=(config&32)?def:nullptr;wf_transition=F(transition);
    WeaponFov_State=state;WeaponFov_Sentinel=sentinel;
    const U32 weapon=(key&1?0u:1u)|((key&2)?2u<<8:0)|((key&4)?3u<<14:0)|((key&8)?4u<<20:0);
    const U32 active=(config&64)?weapon:2u;
    WeaponFov_Defs[0]=def;WeaponFov_Defs[1]=def;WeaponFov_Defs[2]=(config&128)?def:nullptr;
    W(def,0x10,static_cast<Byte*>(props));W(def,0x30,static_cast<const Byte*const*>(table));
    W(props,0x44,int((key&16)?3:0));W(props,0x808,Byte((key&32)!=0));
    W(props,0x6f4,int((key&64)?100:0));W(props,0xbf0,0.25f);W(props,0xbf4,0.5f);
    const float zooms[]={35,20,45,75,65,0,-10,55};
    for(unsigned j=0;j<3;++j){
        W(def,0x270+4*j,zooms[(config>>(8+3*j))&7]);W(sentinel,0x40+4*j,99.0f);
        table[2+j]=(key&(128u<<j))?nullptr:attachments[j];
        for(unsigned k=0;k<3;++k)W(attachments[j],0x40+4*k,(key&(1024u<<k))?99.0f:float(10+5*j+5*k));
        W(attachments[j],0x2c,(config&0x20000)?99.0f:0.2f);W(attachments[j],0x30,(config&0x40000)?99.0f:0.3f);
    }
    W(sentinel,0x2c,99.0f);W(sentinel,0x30,99.0f);
    W(ps,0x18,Byte((key&8192)?2:0));W(ps,0x1d0,int((key&16384)?44:0));W(ps,0x4c,25);
    W(ps,0x1e0,F(fraction));W(ps,0x1b8,active);
    for(unsigned i=0;i<15;++i){W(ps,0x248+i*0x1c,U32(0));W(ps,0x258+i*0x1c,0);}
    if(config&0x80000){W(ps,0x248+14*0x1c,active);W(ps,0x258+14*0x1c,int((config>>20)&3));}
    W(ps,0x5c4,int((config>>22)&7)-3);W(ps,0x5d0,int((config>>25)&3));
    W(ps,0xbec,last);W(ps,0x5d4,int((config>>27)&31));
    W(state,0x4808c,now);W(state,0x88120,last);W(state,0x8811c,F(initial));W(state,0x4ae94,int((key>>15)&1));
    const unsigned kind=config&7;WeaponFov_VariableZoom=kind?&dvar:nullptr;
    W(&dvar,0x18,int(kind==1?1:8));
    if(kind==1)W(&dvar,0x20,Byte((key>>16)&1));else W(&dvar,0x20,values[kind&3]);
    const Byte forWeapon=(key>>17)&1;
    if(image){
        W(image,0xe53a20,WeaponFov_State);W(image,0x265dbb0,WeaponFov_Sentinel);
        W(image,0xd78650,WeaponFov_VariableZoom);
        for(unsigned i=0;i<3;++i)W(image,0x265d3b0+8*i,WeaponFov_Defs[i]);
    }
    const float result=image?reinterpret_cast<float(MS*)(void*,const Byte*,U32,float,Byte)>(static_cast<Byte*>(image)+0xc12f0)(nullptr,ps,weapon,65.0f,forWeapon):GetWeaponViewFov(nullptr,ps,weapon,65.0f,forWeapon);
    trace.result=Bits(result);trace.progress=Read<U32>(state,0x8811c);trace.time=Read<U32>(state,0x88120);
    for(unsigned i=0;i<6;++i){trace.events[i]=wf_calls[i];trace.arguments[i]=wf_args[i];}
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
