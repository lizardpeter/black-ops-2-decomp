// Store MP CG_OffsetVehicleView, 1400C66D0..1400C69EF.
#define T6_SOURCE "cgame/cg_camera.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_vehicle_offset {
using Byte=unsigned char;using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *VehicleOffset_State; // 140E53A20
extern Byte *VehicleOffset_Entities[]; // 140E4F8E0
extern const Byte *VehicleOffset_Weapons[256]; // 14265D3B0
extern const Byte *VehicleOffset_Definitions[256]; // 14265D0B0
extern const Dvar *VehicleOffset_DisableJitter; // 141F04888
extern int VehicleOffset_DemoMode; // 1425A3308
extern int *VehicleOffset_DemoTime; // base 14449EE5C
extern float *VehicleOffset_LastPitch; // base 14449E700
extern const char *VehicleOffset_ErrorText;
bool VehicleOffset_Suppress(); // 1405E07E0
bool VehicleOffset_ErrorCheck(); // 1400A9960
void VehicleOffset_Error(int,const char*); // 14039AEB0
int VehicleOffset_Atoi(const char*);
int VehicleOffset_UsingVehicleWeapon(const Byte*); // 1406468B0
U32 VehicleOffset_SelectWeapon(const Byte*,int); // 14018DA90
U32 VehicleOffset_TurretWeapon(int); // 14018DA10
U32 VehicleOffset_DefaultWeapon(const Byte*); // 1406448B0
int VehicleOffset_Random(); // IAT 1408A8330
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int r;std::memcpy(&r,&v,4);return r;}
static int GetInt(const Dvar*d){
    if(!d)return 0;
    const int t=R<int>(d,0x18);
    if(t==6||t==7)return R<int>(d,0x20);
    if(t==1)return R<Byte>(d,0x20)!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return VehicleOffset_Atoi(R<const char*>(d,0x20));
}
static float FloorOriginal(float x){
    int n=_mm_cvttss_si32(_mm_set_ss(x));
    if(n==(-2147483647-1)||float(n)==x)return x;
    if(_mm_movemask_ps(_mm_set_ss(x))&1)--n;
    return float(n);
}
void OffsetVehicleView(int client,int mode){
    Byte *const cg=VehicleOffset_State;
    const Byte *ps=cg+0x480a8;
    const bool suppress=VehicleOffset_Suppress();
    for(unsigned i=0;i<6;++i)W(cg,0x6b69c+4*i,U32(0));
    const int n=R<std::int16_t>(ps,0x474);
    if(VehicleOffset_ErrorCheck())VehicleOffset_Error(1,VehicleOffset_ErrorText);
    const Byte *ent=VehicleOffset_Entities[client]+n*0x3d8;
    const int seat=R<int>(ps,0x478);
    const bool jitter=seat==0?(R<Byte>(ent,0x1f8)&0x40)!=0:
        (U32(int(R<std::int16_t>(ent,0x284)))&(32u<<((U32(seat)-1u)&31)))!=0;
    if(jitter&&!GetInt(VehicleOffset_DisableJitter)){
        U32 weapon=0;
        if(VehicleOffset_UsingVehicleWeapon(ps))weapon=VehicleOffset_SelectWeapon(ps,client);
        else if(R<U32>(ps,0xfc)&0x300)weapon=VehicleOffset_TurretWeapon(client);
        if(!(weapon&255))weapon=VehicleOffset_DefaultWeapon(ps);
        const Byte *props=R<const Byte*>(VehicleOffset_Weapons[weapon&255],0x10);
        if(props&&!suppress){
            const float a=(float(VehicleOffset_Random())*F(0x38800000)-1.0f)*R<float>(props,0xb2c);
            W(cg,0x649d0,a+R<float>(cg,0x649d0));
            const float b=(float(VehicleOffset_Random())*F(0x38800000)-1.0f)*R<float>(props,0xb28);
            W(cg,0x649d4,b+R<float>(cg,0x649d4));
        }
    }
    if(mode!=2)return;
    const Byte *def=VehicleOffset_Definitions[R<Byte>(ent,0x286)];
    bool reset=false;
    if(VehicleOffset_DemoMode==2){
        const int now=R<int>(cg,0x4808c),prev=VehicleOffset_DemoTime[client];
        reset=prev>now||Diff(now,prev)>100;VehicleOffset_DemoTime[client]=now;
    }
    const unsigned type=R<unsigned short>(def,8);
    float pitch=R<float>(cg,0x649d0);
    if(type!=2&&R<unsigned short>(def,0x28)!=4&&!reset&&!R<int>(def,0xba4)){
        const float turns=(VehicleOffset_LastPitch[client]-pitch)*F(0x3b360b61);
        const float delta=((turns-FloorOriginal(turns+0.5f))*360.0f)*(type==1?0.9f:0.75f);
        const float clamped=delta>=-30.0f?_mm_cvtss_f32(_mm_min_ss(_mm_set_ss(delta),_mm_set_ss(30.0f))):-30.0f;
        pitch+=clamped;W(cg,0x649d0,pitch);
    }
    VehicleOffset_LastPitch[client]=pitch;
}
}
#endif
