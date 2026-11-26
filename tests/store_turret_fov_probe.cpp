#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_turret_fov_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_turret_fov {
static Byte state[0x4b000],ps[0x200],defs[2][0x280],weaponProps[2][0x810],attachments[3][0x48],sentinel[0x48];
static const Byte *table[64];
static unsigned count;
static U32 weapon1,weapon2;
struct Trace {U count;U events[2];U result;U transition;U argument;};
static Trace trace{};
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static U32 Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
extern "C" {
const Byte *Turret_State,*Turret_WeaponDefs[256],*Turret_AttachmentSentinel;
__attribute__((visibility("hidden"))) U32 turret_transition_kind=0,turret_transition_argument=0;
__attribute__((visibility("hidden"))) float turret_transition_a=0,turret_transition_b=0;
U32 Turret_SelectWeapon(int client){assert(count<2);trace.events[count]=U(client);return count++?weapon2:weapon1;}
float Turret_TransitionA(U32 w){turret_transition_kind=1;turret_transition_argument=w;return turret_transition_a;}
float Turret_TransitionB(U32 w){turret_transition_kind=2;turret_transition_argument=w;return turret_transition_b;}
}
static U32 MS MSelect(int c){return Turret_SelectWeapon(c);}
// LTCG originals preserve the caller's live XMM1..4. These exact boundary
// stubs intentionally avoid any compiler-generated clobber of those lanes.
extern "C" __attribute__((ms_abi,naked)) float TurretMockA(U32){
    __asm__("movl $1,turret_transition_kind(%rip)\n\tmovl %ecx,turret_transition_argument(%rip)\n\tmovss turret_transition_a(%rip),%xmm0\n\tret");
}
extern "C" __attribute__((ms_abi,naked)) float TurretMockB(U32){
    __asm__("movl $2,turret_transition_kind(%rip)\n\tmovl %ecx,turret_transition_argument(%rip)\n\tmovss turret_transition_b(%rip),%xmm0\n\tret");
}
extern "C" {
void *TurretMock(unsigned i){void*m[]={reinterpret_cast<void*>(MSelect),reinterpret_cast<void*>(TurretMockA),reinterpret_cast<void*>(TurretMockB)};assert(i<3);return m[i];}
unsigned TurretTraceSize(){return sizeof(Trace);}
void TurretRun(void*image,unsigned key,U32 frac,U32 zoom,U32 duration,U32 sentinelBits,void*out){
    count=0;trace={};turret_transition_kind=0;turret_transition_argument=0;
    const int client=key&1;
    weapon1=1u|((key&2)?2u<<8:0)|((key&4)?3u<<14:0)|((key&8)?4u<<20:0);
    weapon2=(key&16)?2:1;
    Turret_State=state;W(state,0x4ae94,int((key&32)!=0));W(ps,0x1e0,F(frac));
    for(unsigned i=0;i<2;++i){
        Turret_WeaponDefs[i+1]=defs[i];
        W(defs[i],0x10,static_cast<Byte*>(weaponProps[i]));
        W(defs[i],0x30,static_cast<const Byte *const *>(table));
        W(defs[i],0x270,F(zoom));
        W(weaponProps[i],0x808,Byte((key&(64u<<i))!=0));
    }
    Turret_AttachmentSentinel=sentinel;W(sentinel,0x40,F(sentinelBits));
    for(unsigned i=0;i<3;++i){
        table[2+i]=(key&(256u<<i))?nullptr:attachments[i];
        W(attachments[i],0x40,(key&(2048u<<i))?F(sentinelBits):float(20+20*i));
    }
    turret_transition_a=F(duration);turret_transition_b=F(duration);
    if(image){
        W(image,0xe53a20,Turret_State);
        W(image,0x265d3b8,Turret_WeaponDefs[1]);W(image,0x265d3c0,Turret_WeaponDefs[2]);
        W(image,0x265dbb0,Turret_AttachmentSentinel);
    }
    float result=image?reinterpret_cast<float(MS*)(int,const Byte*,float)>(static_cast<Byte*>(image)+0xc1a00)(client,ps,123.0f):GetTurretViewFov(client,ps,123.0f);
    trace.count=count;trace.result=Bits(result);trace.transition=turret_transition_kind;trace.argument=turret_transition_argument;
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
