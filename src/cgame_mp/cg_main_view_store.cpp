// Store MP CG_CalcViewValues, 14016C510..14016CE7D (538 instructions).
// Complete enclosing-body candidate. Unresolved callees have address names;
// fields are kept at original x64 byte offsets until layouts are recovered.
#define T6_SOURCE "cgame_mp/cg_view_mp.cpp"
#if defined(T6_STORE) && defined(T6_STORE_CAMERA_REPLAY)
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <xmmintrin.h>
namespace store_main_view {
using Byte=unsigned char;
using U32=std::uint32_t;
struct Dvar;
extern "C" {
extern Byte *View_State; // 140E53A20
extern const Byte *View_Static; // 140E53B70
extern const Dvar *View_AdaptiveSkin; // 140E5A650
extern const Dvar *View_OverflowScale; // 1427E6CA8
extern const Dvar *View_HelicopterCull; // 140D78610
extern const Dvar *View_SmoothDuration; // 140E4BA58
extern const Dvar *View_AdsScale; // 140E49A20
extern const Byte *View_SkinPressure; // array base 1427F1D6C
extern const Byte *View_BlurA; // array base 143BFD290, stride 0x1c
extern const Byte *View_ClientMapping; // array base 141EF91E4, stride 0x18
extern const Byte *View_MenuBlur; // array base 1443C54FC, stride 0x24
extern const Byte *View_BlurB; // array base 140E4F91C, stride 0x4130
extern float View_GlobalBlur; // 1421214EC
extern U32 View_BlurFlags; // 140E61794
extern int View_BlurEnabled; // 142CB7DCC
extern int View_DemoMode; // 1425A3308
extern Byte View_DemoFlags,View_PlaybackInited; // 1425A330C,142653908
extern const Byte *View_Playback; // 142653900
extern Byte **View_TagObject; // 140D76830
extern unsigned short View_TagId; // 141EF6B1A
int View_Atoi(const char*); // IAT 1408A7DC0
double View_Atof(const char*); // IAT 1408A7DD8
int View_UpdateMode(int); // 1400C0E20
void View_CalcFov(int,float); // 1400C2060
void View_ApplyVision(int,Byte); // 140173ED0
void View_Cubemap(Byte*); // 14016BDA0
void View_AnglesToAxis(const float*,float*); // 140464D00
void View_Linked(int,int); // 140100FC0
void View_LastStandSway(); // 14016B450, first argument not prepared
void View_Vehicle(int); // 1400C4490
void View_VehicleThirdPerson(int); // 1400C5FA0
void View_Missile(int); // 1400C3AE0
void View_Spike(); // 1400C3390
void View_Sub_140621C10(const Byte*,float*);
void View_RemoteEyes(int); // 1400C3950
void View_Turret(int); // 14016BFB0
void View_Gunner(int,Byte*); // 1400C2740
void View_FirstPerson(Byte*); // 14016BA40
void View_OffsetVehicle(int,int); // 1400C66D0
void View_Shake(int); // 1400C7540
void View_Velocity(Byte*); // 14016C380
void View_Tween(int); // 1400BF940
void View_MissileAngles(int); // 1400C3E10
int View_UsingVehicleWeapon(const Byte*); // 1406468B0
U32 View_Weapon(const Byte*); // 1406448B0
void View_Sub_14017F0B0(Byte*);
Byte *View_Sub_1400E6370(Byte*,Byte*,unsigned);
void View_Sub_1400DE970(Byte*,float*); // caller retains RCX
void View_AxisToAngles(const float*,float*); // 140462D70
void View_ThirdPerson(int); // 14016A940
void View_Perturb(Byte*); // 140157BC0
}
template<class T>static T R(const void*p,std::ptrdiff_t o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 b){float f;std::memcpy(&f,&b,4);return f;}
static int Diff(int a,int b){U32 v=U32(a)-U32(b);int i;std::memcpy(&i,&v,4);return i;}
static int Truncate(float f){return _mm_cvttss_si32(_mm_set_ss(f));}
static float Sqrt(float f){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(f)));}
static Byte Bool(const Dvar*d){
    if(!d)return 0;
    if(R<int>(d,0x18)==1)return R<Byte>(d,0x20);
    return View_Atoi(R<const char*>(d,0x20))!=0;
}
static float Float(const Dvar*d){
    if(!d)return 0;
    const int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(View_Atof(R<const char*>(d,0x20)));
}
static void Copy(Byte*d,unsigned dst,const Byte*s,unsigned src,unsigned words=3){
    for(unsigned i=0;i<words;++i)W(d,dst+4*i,R<U32>(s,src+4*i));
}
static float* Floats(Byte*p,unsigned off){return reinterpret_cast<float*>(p+off);}
static void CopyPlayerOrigin(Byte*cg){Copy(cg,0x4d924,cg,0x480d0);}
static void CopyPlayerAngles(Byte*cg){Copy(cg,0x649d0,cg,0x482a0);}
static void Axis(Byte*cg){View_AnglesToAxis(Floats(cg,0x649d0),Floats(cg,0x4d934));}
void CalcViewValues(int client){
    Byte *const cg=View_State;
    const Dvar *adaptive=View_AdaptiveSkin;
    W(cg,0x4d95c,R<int>(cg,0x4808c));W(cg,0x4d960,U32(0));
    W(cg,0x646e4,client);W(cg,0x4e82c,1.0f);
    W(cg,0x64888,Byte(0));W(cg,0x6488c,1.0f);
    float skin=1.0f;
    if(Bool(adaptive)){
        skin=R<float>(cg,0x64890);
        const float overflow=Float(View_OverflowScale);
        const float pressure=R<float>(View_SkinPressure,std::ptrdiff_t(U32(client))*4);
        if(pressure>F(0x3f666666))skin=overflow;
        else if(!(pressure>=0.1f))skin=1.0f;
    }
    W(cg,0x64890,skin);
    const float a=R<float>(View_BlurA,std::ptrdiff_t(client)*0x1c);
    const auto mapping=std::ptrdiff_t(client)*0x18;
    const int menu=R<int>(View_ClientMapping,mapping)==client?R<int>(View_ClientMapping,mapping+4):-1;
    const float global=View_GlobalBlur;
    float b=R<float>(View_MenuBlur,std::ptrdiff_t(menu)*0x24);
    if(client!=-1 && (View_BlurFlags&8) && View_BlurEnabled)
        b=Sqrt(global*global+b*b);
    const float c=R<float>(View_BlurB,std::ptrdiff_t(client)*0x4130);
    W(cg,0x4db48,Sqrt((c*c+a*a)+b*b));
    const int mode=View_UpdateMode(client);
    View_CalcFov(client,-1.0f);View_ApplyVision(client,0);
    if(R<int>(cg,0xc)){View_CalcFov(client,-1.0f);View_Cubemap(cg);return;}
    const Byte *statics=View_Static;
    Byte *view=View_State;
    Copy(view,0x4d8f0,statics,0,4);W(view,0x646e0,Byte(0));
    Copy(view,0x4d900,statics,0x14,4);
    if(R<Byte>(cg,0x481a8)&0xe)W(cg,0x64888,Bool(View_HelicopterCull));
    const int pm=R<int>(cg,0x480ac);
    const int demo=View_DemoMode;
    const Byte playback=View_PlaybackInited;
    const Byte *play=View_Playback;
    if(pm==6 || (demo==2 && pm==5 &&
        (!playback || R<int>(play,0xd4c00)!=2) && R<int>(cg,0x482ec)<2)){
        CopyPlayerOrigin(cg);CopyPlayerAngles(cg);Axis(cg);return;
    }
    view=View_State;
    if((R<int>(view,0x6b818) || (demo==2 && playback && (View_DemoFlags&2) &&
          R<Byte>(play,0x26a998))) && R<int>(view,0x6b82c)){
        View_Linked(R<int>(cg,0x6b82c),client);View_ApplyVision(client,0);return;
    }
    const float bob=float(R<Byte>(cg,0x480b0))*F(0x3c49d9b5);
    W(cg,0x6b694,(bob+bob)+F(0x40c90fdb));
    float speed;
    if(R<Byte>(cg,0x480b4)&8)
        speed=Diff(R<int>(cg,0x4808c),R<int>(cg,0x48168))<500?0.0f:R<float>(cg,0x480e4);
    else {
        const float x=R<float>(cg,0x480dc),y=R<float>(cg,0x480e0);
        speed=Sqrt(x*x+y*y);
    }
    W(cg,0x6b698,speed);
    View_LastStandSway();
    CopyPlayerOrigin(cg);
    if(!R<Byte>(cg,0x83484) && !R<int>(R<const Byte*>(cg,0x30),0x14) &&
        !R<int>(cg,0x480a0)){
        float height=R<float>(cg,0x83490);
        if(height<0.0f || height>0.0f){
            const int elapsed=Diff(R<int>(cg,0x4808c),R<int>(cg,0x8348c));
            if(elapsed>=0){
                const Dvar *duration=View_SmoothDuration;
                float fraction=1.0f;
                if(duration){
                    const int type=R<int>(duration,0x18);
                    const int ms=Truncate(Float(duration)*1000.0f);
                    if(type!=2 && type!=6)height=R<float>(cg,0x83490);
                    if(elapsed<ms)fraction=float(elapsed)/float(ms);
                }
                W(cg,0x4d92c,R<float>(cg,0x4d92c)-(1.0f-fraction)*height);
            }
        }
    }
    CopyPlayerAngles(cg);
    switch(mode){
    case 2:View_Vehicle(client);CopyPlayerAngles(cg);break;
    case 3:View_VehicleThirdPerson(client);break;
    case 6:View_Missile(client);break;
    case 7:View_Spike();break;
    }
    view=View_State;
    const int currentPm=R<int>(view,0x480ac);
    if((currentPm==1 || currentPm==11 || (R<Byte>(view,0x4852c)&4)) &&
       !(R<U32>(view,0x481a4)&0x4300))
        View_Sub_140621C10(view+0x48530,Floats(cg,0x649d0));
    if(mode!=2 && mode!=3){
        const float t=(100.0f-float(Diff(R<int>(cg,0x4808c),R<int>(cg,0x4aec0))))*0.01f;
        if(t>0.0f && t<1.0f)
            for(unsigned i=0;i<3;++i)W(cg,0x4d924+4*i,t*R<float>(cg,0x4aec4+4*i)+R<float>(cg,0x4d924+4*i));
        else W(cg,0x4aec0,0);
    }
    if(R<Byte>(cg,0x4852c)&4)View_RemoteEyes(client);
    switch(mode){
    case 5:View_Turret(client);break;
    case 2:case 3:View_OffsetVehicle(client,mode);break;
    case 4:W(cg,0x64888,Bool(View_HelicopterCull));View_Gunner(client,cg);break;
    case 6:case 7:break;
    default:if(R<int>(cg,0x480a0)!=1)View_FirstPerson(cg);break;
    }
    View_Shake(client);View_CalcFov(client,-1.0f);
    View_Velocity(cg);View_Tween(client);
    if(mode==6){View_MissileAngles(client);Copy(cg,0x649e8,cg,0x649d0);}
    Axis(cg);
    view=View_State;
    if(!(R<U32>(view,0x481a4)&0x300) && !View_UsingVehicleWeapon(view+0x480a8) &&
       R<int>(view,0x480a0)!=1 && (View_Weapon(view+0x480a8)&255u)){
        Byte **const slot=View_TagObject;
        if(*slot){
            Copy(view,0x6b6e4,view,0x4d934,9);
            Copy(view,0x6b708,view,0x4d924);
            View_Sub_14017F0B0(*slot);
            (*slot)[0x79]|=0x40;
            Byte *tag=View_Sub_1400E6370(view+0x7f2b8,*slot,View_TagId);
            if(tag){
                View_Sub_1400DE970(tag,Floats(view,0x4d934));
                Copy(view,0x4d924,tag,0x10);
                (*slot)[0x79]&=0xbf;
                View_AxisToAngles(Floats(view,0x4d934),Floats(view,0x649d0));
            }else (*slot)[0x79]&=0xbf;
        }
    }
    if(R<int>(cg,0x480a0)==1 && (mode==5 || mode<=1 || mode>=9))View_ThirdPerson(client);
    View_Perturb(cg);
    const float scale=Float(View_AdsScale);
    const float ads=R<float>(cg,0x48288);
    const float z=scale-ads*ads;
    W(cg,0x648e0,z>=1.0f?1.0f:z);
}
}
#endif
