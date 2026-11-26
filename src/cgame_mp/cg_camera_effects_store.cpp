// Store MP complete first-person view and camera shake bodies.
// Original offsets and direct call boundaries are retained.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <xmmintrin.h>
namespace store_camera_effects {
using Byte=unsigned char;
using U32=std::uint32_t;
struct BobInput {
    const Byte *ps;
    int previousDelta,delta;
    float a,b,speed,dt,c;
};
static_assert(offsetof(BobInput,previousDelta)==8 && offsetof(BobInput,c)==32);
extern "C" {
extern Byte *Effects_State; // 140E53A20
extern Byte *Effects_Shakes; // original array at 143BFB000, stride 0x94
void Effects_BobAngles(const BobInput*,float*,int); // 140660B40
float Effects_VerticalBob(const Byte*,float,float); // 1406602A0
float Effects_HorizontalBob(const Byte*,float,float); // 140660470
__m128 Effects_SinCosPair(float,float); // 140890DA3
__m128 Effects_SinCosSingle(float); // 140890D60
void Effects_OffsetOrigin(float*,float,float); // 140472FE0
int Effects_UpdateShake(Byte*,Byte*); // 1400C72B0
int Effects_Random(); // IAT 1408A8330
float Effects_Sin(float); // 140870BC8
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 b){float f;std::memcpy(&f,&b,4);return f;}
static float Neg(float f){U32 b;std::memcpy(&b,&f,4);return F(b^0x80000000u);}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int i;std::memcpy(&i,&v,4);return i;}
// 14016BA40..14016BD96.
void OffsetFirstPersonView(Byte *cg) {
    if(R<int>(R<const Byte*>(cg,0x30),0x14)==6 || (R<U32>(cg,0x481a4)&0x300))return;
    const Byte *ps=cg+0x480a8;
    const int base=R<int>(cg,0x48c94),now=R<int>(cg,0x4808c),previous=R<int>(cg,0x6b650);
    const BobInput input{ps,previous?Diff(previous,base):0,Diff(now,base),
        R<float>(cg,0x6b688),R<float>(cg,0x6b68c),R<float>(cg,0x6b698),
        float(R<int>(cg,0x48088))*F(0x3a83126f),R<float>(cg,0x4aea4)};
    float angles[3];
    Effects_BobAngles(&input,angles,0);
    for(unsigned i=0;i<3;++i)W(cg,0x649d0+4*i,angles[i]+R<float>(cg,0x649d0+4*i));
    float z=R<float>(cg,0x4d92c);
    z+=R<int>(cg,0x481f4)==0x3ff?R<float>(cg,0x482b0):20.0f;
    W(cg,0x4d92c,z);
    const float vertical=Effects_VerticalBob(ps,R<float>(cg,0x6b694),R<float>(cg,0x6b698));
    W(cg,0x4d92c,vertical+R<float>(cg,0x4d92c));
    const float horizontal=Effects_HorizontalBob(ps,R<float>(cg,0x6b694),R<float>(cg,0x6b698));
    alignas(16) float pair[4],roll[4];
    _mm_store_ps(pair,Effects_SinCosPair(R<float>(cg,0x649d4)*F(0x3c8efa35),R<float>(cg,0x649d0)*F(0x3c8efa35)));
    _mm_store_ps(roll,Effects_SinCosSingle(R<float>(cg,0x649d8)*F(0x3c8efa35)));
    const float sy=pair[0],cy=pair[1],sp=pair[2],cp=pair[3],sr=roll[0],cr=roll[1];
    const float s=sp*sr;
    W(cg,0x4d92c,Neg(cp*sr)*horizontal+R<float>(cg,0x4d92c));
    W(cg,0x4d928,(Neg(cr*cy)-s*sy)*horizontal+R<float>(cg,0x4d928));
    W(cg,0x4d924,(cr*sy-s*cy)*horizontal+R<float>(cg,0x4d924));
    const float elapsed=float(Diff(R<int>(cg,0x4808c),R<int>(cg,0x4d8dc)));
    if(elapsed>0.0f) {
        float step=0;
        if(elapsed<150.0f)step=elapsed*F(0x3bda740e);
        else if(elapsed<450.0f)step=1.0f-(elapsed-150.0f)*F(0x3b5a740e);
        if(elapsed<450.0f)W(cg,0x4d92c,step*R<float>(cg,0x4d8d8)+R<float>(cg,0x4d92c));
    }
    Effects_OffsetOrigin(reinterpret_cast<float*>(cg+0x4d924),R<float>(cg,0x649d4),R<float>(cg,0x48138));
    const float minimum=R<float>(cg,0x480d8)+8.0f;
    if(minimum>R<float>(cg,0x4d92c))W(cg,0x4d92c,minimum);
}
// 1400C7540..1400C7713. Four active shake slots; the strongest amplitude
// chooses the frequency multiplier. Inactive frames refresh the phase.
void ShakeCamera(int client) {
    Byte *cg=Effects_State,*slots=Effects_Shakes+client*0x94;
    const float time=float(R<int>(cg,0x4808c));
    float strongest=0,frequency=0;
    for(unsigned i=0;i<4;++i){
        Byte *slot=slots+i*0x24;
        if(Effects_UpdateShake(cg,slot)){
            const float a=R<float>(slot,0x1c);
            if(a>strongest){strongest=a;frequency=R<float>(slot,0x20);}
        }
    }
    const float extra=R<float>(cg,0x6b714);
    if(extra>strongest)frequency=extra;
    float amount=extra>strongest?extra:strongest;
    if(!(amount>0)){
        W(slots,0x90,float(Effects_Random())*F(0x39490fdb)-F(0x40490fdb));return;
    }
    if(amount>1)amount=1;
    const U32 rates[]={0x3d2b92a7,0x3da0d97c,0x3d80adfd};
    const float amplitudes[]={18,16,10};
    for(unsigned i=0;i<3;++i){
        const float wave=Effects_Sin(time*F(rates[i])+R<float>(slots,0x90));
        W(cg,0x649d0+i*4,((wave*frequency)*amplitudes[i])*amount+R<float>(cg,0x649d0+i*4));
    }
}
}
#endif
