#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_camera_effects_store.cpp"
#include <cassert>
#include <cmath>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_effects {
alignas(16) static Byte state[0x6b800],snap[32],shakes[0x128];
static unsigned key;
struct Trace{U n;U events[16][8];U outputs[12];};
static Trace trace;
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0,U e=0,U f=0,U g=0){
    assert(trace.n<16);auto *p=trace.events[trace.n++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;p[5]=e;p[6]=f;p[7]=g;
}
extern "C" {
Byte *Effects_State,*Effects_Shakes;
void Effects_BobAngles(const BobInput*p,float*a,int v){assert(p->ps==state+0x480a8&&v==0);Push(1,U32(p->previousDelta),U32(p->delta),Bits(p->a),Bits(p->b),Bits(p->speed),Bits(p->dt),Bits(p->c));a[0]=2;a[1]=-3;a[2]=4;}
float Effects_VerticalBob(const Byte*p,float a,float b){assert(p==state+0x480a8);Push(2,Bits(a),Bits(b));return 1.25f;}
float Effects_HorizontalBob(const Byte*p,float a,float b){assert(p==state+0x480a8);Push(3,Bits(a),Bits(b));return -2.5f;}
__m128 Effects_SinCosPair(float a,float b){Push(4,Bits(a),Bits(b));return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
__m128 Effects_SinCosSingle(float a){Push(5,Bits(a));return _mm_setr_ps(std::sin(a),std::cos(a),0,0);}
void Effects_OffsetOrigin(float*p,float a,float b){assert(reinterpret_cast<Byte*>(p)==state+0x4d924);Push(6,Bits(a),Bits(b));p[0]+=1;p[1]-=2;p[2]-=3;}
int Effects_UpdateShake(Byte*p,Byte*s){assert(p==state);unsigned n=(s-Effects_Shakes)/0x24;Push(7,n);return (key>>n)&1;}
int Effects_Random(){Push(8);return int(key&32767);}
float Effects_Sin(float f){Push(9,Bits(f));return std::sin(f);}
}
static void MS MBob(const BobInput*p,float*a,int v){Effects_BobAngles(p,a,v);}
static float MS MVertical(const Byte*p,float a,float b){return Effects_VerticalBob(p,a,b);}
static float MS MHorizontal(const Byte*p,float a,float b){return Effects_HorizontalBob(p,a,b);}
static __m128 MS MPair(float a,float b){return Effects_SinCosPair(a,b);}
static __m128 MS MSingle(float a){return Effects_SinCosSingle(a);}
static void MS MOffset(float*p,float a,float b){Effects_OffsetOrigin(p,a,b);}
static int MS MUpdate(Byte*p,Byte*s){return Effects_UpdateShake(p,s);}
static int MS MRandom(){return Effects_Random();}
static float MS MSin(float a){return Effects_Sin(a);}
extern "C" {
void* EffectsMock(unsigned i){void*m[]={reinterpret_cast<void*>(MBob),reinterpret_cast<void*>(MVertical),reinterpret_cast<void*>(MHorizontal),reinterpret_cast<void*>(MPair),reinterpret_cast<void*>(MSingle),reinterpret_cast<void*>(MOffset),reinterpret_cast<void*>(MUpdate),reinterpret_cast<void*>(MRandom),reinterpret_cast<void*>(MSin)};assert(i<9);return m[i];}
unsigned EffectsSize(){return sizeof(Trace);}
void EffectsRun(void*image,unsigned op,U32 seed,int elapsed,U32 extra,void*out){
    key=seed;trace={};Effects_State=state;Effects_Shakes=shakes;
    W(state,0x30,static_cast<Byte*>(snap));W(snap,0x14,int((key&1)?6:0));W(state,0x481a4,U32((key&2)?0x300:0));
    W(state,0x4808c,1000);W(state,0x48c94,700);W(state,0x6b650,int((key&4)?900:0));W(state,0x48088,16);
    W(state,0x6b688,1.5f);W(state,0x6b68c,2.5f);W(state,0x6b698,20.0f);W(state,0x6b694,3.0f);W(state,0x4aea4,0.25f);
    W(state,0x481f4,int((key&8)?0x3ff:0));W(state,0x482b0,60.0f);W(state,0x480d8,(key&16)?100.0f:0.0f);
    W(state,0x48138,12.0f);W(state,0x4d8dc,1000-elapsed);W(state,0x4d8d8,5.0f);W(state,0x6b714,extra);
    for(unsigned i=0;i<3;++i){W(state,0x649d0+4*i,float(int((key>>(5+i*5))&31)-16)*15);W(state,0x4d924+4*i,float(10+i));}
    for(unsigned i=0;i<4;++i){W(shakes,0x24*i+0x1c,float((key>>(8+3*i))&7)*0.25f);W(shakes,0x24*i+0x20,float(i+1)*0.5f);}
    W(shakes,0x90,0.25f);
    if(image){
        W(image,0xe53a20,Effects_State);std::memcpy(static_cast<Byte*>(image)+0x3bfb000,shakes,0x94);
        if(op==0)reinterpret_cast<void(MS*)(Byte*)>(static_cast<Byte*>(image)+0x16ba40)(state);
        else {
            Effects_Shakes=static_cast<Byte*>(image)+0x3bfb000;
            reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0xc7540)(0);
            std::memcpy(shakes,Effects_Shakes,0x94);
        }
    }else if(op==0)OffsetFirstPersonView(state);else ShakeCamera(0);
    for(unsigned i=0;i<3;++i){trace.outputs[i]=R<U32>(state,0x649d0+4*i);trace.outputs[i+3]=R<U32>(state,0x4d924+4*i);}
    trace.outputs[6]=R<U32>(shakes,0x90);std::memcpy(out,&trace,sizeof(trace));
}
}
}
