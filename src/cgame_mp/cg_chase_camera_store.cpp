// Store MP CG_OffsetChaseCamView, 14016A5D0..14016A930.
// Preserve the original local-client view angle normalization and trace.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_chase_camera {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern const Dvar *Chase_Mode,*Chase_Angle,*Chase_Range;
extern Byte *Chase_State,*Chase_ClientState;
extern int Chase_ClientCount,Chase_DemoMode;
extern Byte Chase_DemoFlag,Chase_PlaybackInited;
extern const Byte *Chase_Playback;
int Chase_Atoi(const char*);
double Chase_Atof(const char*);
void Chase_DemoUpdate(int); // 1405CF280
__m128 Chase_SinCos(float,float); // 140890DA3
float Chase_Atan2(float,float); // 140870BBC
void Chase_Trace(Byte*,float*,const float*,int,float*,Byte,Byte); // 1400C69F0
void Chase_Axis(const float*,float*); // 140464D00
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
static float F(U32 u){float f;std::memcpy(&f,&u,4);return f;}
static int GetInt(const Dvar*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==6||t==7)return R<int>(d,0x20);
    if(t==1)return R<Byte>(d,0x20)!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Chase_Atoi(R<const char*>(d,0x20));
}
static float GetFloat(const Dvar*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(Chase_Atof(R<const char*>(d,0x20)));
}
static float FloorOriginal(float x){
    int n=_mm_cvttss_si32(_mm_set_ss(x));
    if(n==(-2147483647-1))return x;
    if(float(n)==x)return x;
    if(_mm_movemask_ps(_mm_set_ss(x))&1)--n;
    return float(n);
}
void OffsetChaseCamView(int client){
    const int mode=GetInt(Chase_Mode);
    Byte *const cg=Chase_State;
    if(Chase_DemoMode==2 && (Chase_DemoFlag ||
        (Chase_PlaybackInited && R<int>(Chase_Playback,0x1017a4)==4)))
        Chase_DemoUpdate(client);
    const Byte *cl=client<Chase_ClientCount?Chase_ClientState:nullptr;
    float angles[3]={R<float>(cl,0x2ca4),R<float>(cl,0x2ca8),R<float>(cl,0x2cac)};
    if(mode==1)angles[1]+=R<float>(cg,0x649d4);
    const float turns=angles[1]*F(0x3b360b61);
    angles[1]=(turns-FloorOriginal(turns+0.5f))*360.0f;
    alignas(16) float t[4];
    _mm_store_ps(t,Chase_SinCos(angles[1]*F(0x3c8efa35),angles[0]*F(0x3c8efa35)));
    float *origin=reinterpret_cast<float*>(cg+0x4d924);
    const float x=origin[0],y=origin[1],z=origin[2];
    const float sy=t[0]*t[3],cy=t[1]*t[3];
    float horizontal=_mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss((sy*sy+cy*cy)*262144.0f)));
    if(!(horizontal>=1.0f))horizontal=1.0f;
    const float pitch=Chase_Atan2(t[2]*-512.0f,horizontal);
    const float yaw=angles[1]-GetFloat(Chase_Angle);
    _mm_store_ps(t,Chase_SinCos(yaw*F(0x3c8efa35),pitch*F(0xbf7fffff)));
    const float range=-GetFloat(Chase_Range);
    const float target[3]={range*(t[1]*t[3])+x,y+range*(t[0]*t[3]),z+range*(-t[2])};
    Chase_Trace(cg,origin,target,0x2818011,origin,1,1);
    Chase_Axis(angles,reinterpret_cast<float*>(cg+0x4d934));
}
}
#endif
