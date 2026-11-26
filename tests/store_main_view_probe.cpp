#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_main_view_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_main_view {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x88200],snap[0x20],play[0x26a999],statics[0x24],obj[0x100],tag[0x20];
static Byte pressure[4],blurA[4],mapping[8],blurB[4],menus[0x6c];
static Byte *object;
static Dvar dvars[5];
static const char *strings[]={"0","1","-1","0.25","2","nan","inf","0.9"};
static unsigned key;static int mode;
struct Event{U kind,a,b,c;};
struct Trace{U count;Event events[48];U fields[64];};
static Trace trace;
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<48);trace.events[trace.count++]={k,a,b,c};}
extern "C" {
Byte *View_State;
const Byte *View_Static;
const Dvar *View_AdaptiveSkin,*View_OverflowScale,*View_HelicopterCull,*View_SmoothDuration,*View_AdsScale;
const Byte *View_SkinPressure,*View_BlurA,*View_ClientMapping,*View_MenuBlur,*View_BlurB;
float View_GlobalBlur;
U32 View_BlurFlags;
int View_BlurEnabled,View_DemoMode;
Byte View_DemoFlags,View_PlaybackInited;
const Byte *View_Playback;
Byte **View_TagObject;
unsigned short View_TagId;
int View_Atoi(const char*s){unsigned i=0;while(i<8&&s!=strings[i])++i;assert(i<8);Push(30,i);return std::atoi(s);}
double View_Atof(const char*s){unsigned i=0;while(i<8&&s!=strings[i])++i;assert(i<8);Push(31,i);return std::atof(s);}
int View_UpdateMode(int c){Push(1,c);return mode;}
void View_CalcFov(int c,float f){Push(2,c,Bits(f));}
void View_ApplyVision(int c,Byte b){Push(3,c,b);}
void View_Cubemap(Byte*p){assert(p==state);Push(4);}
void View_AnglesToAxis(const float*a,float*b){assert(a==Floats(state,0x649d0)&&b==Floats(state,0x4d934));Push(5,Bits(a[0]),Bits(a[1]),Bits(a[2]));for(unsigned i=0;i<9;++i)b[i]=a[i%3]+float(i);}
void View_Linked(int a,int c){Push(6,a,c);}
void View_LastStandSway(){Push(7);}
void View_Vehicle(int c){Push(8,c);}
void View_VehicleThirdPerson(int c){Push(9,c);}
void View_Missile(int c){Push(10,c);}
void View_Spike(){Push(11);}
void View_Sub_140621C10(const Byte*p,float*a){assert(p==state+0x48530&&a==Floats(state,0x649d0));Push(12);}
void View_RemoteEyes(int c){Push(13,c);}
void View_Turret(int c){Push(14,c);}
void View_Gunner(int c,Byte*p){assert(p==state);Push(15,c);}
void View_FirstPerson(Byte*p){assert(p==state);Push(16);}
void View_OffsetVehicle(int c,int m){Push(17,c,m);}
void View_Shake(int c){Push(18,c);}
void View_Velocity(Byte*p){assert(p==state);Push(19);}
void View_Tween(int c){Push(20,c);}
void View_MissileAngles(int c){Push(21,c);}
int View_UsingVehicleWeapon(const Byte*p){assert(p==state+0x480a8);Push(22);return (key>>20)&1;}
U32 View_Weapon(const Byte*p){assert(p==state+0x480a8);Push(23);return (key>>21)&1;}
void View_Sub_14017F0B0(Byte*p){assert(p==obj);Push(24);}
Byte* View_Sub_1400E6370(Byte*p,Byte*q,unsigned id){assert(p==state+0x7f2b8&&q==obj);Push(25,id,obj[0x79]);return (key&(1u<<23))?tag:nullptr;}
void View_Sub_1400DE970(Byte*p,float*b){assert(p==tag&&b==Floats(state,0x4d934));Push(26);for(unsigned i=0;i<9;++i)b[i]=float(i+50);}
void View_AxisToAngles(const float*a,float*b){assert(a==Floats(state,0x4d934)&&b==Floats(state,0x649d0));Push(27);for(unsigned i=0;i<3;++i)b[i]=a[i];}
void View_ThirdPerson(int c){Push(28,c);}
void View_Perturb(Byte*p){assert(p==state);Push(29);}
}
static int MS MUpdateMode(int c){return View_UpdateMode(c);}
static void MS MCalcFov(int c,float f){View_CalcFov(c,f);}
static void MS MApplyVision(int c,Byte b){View_ApplyVision(c,b);}
static void MS MCubemap(Byte*p){View_Cubemap(p);}
static void MS MAnglesToAxis(const float*a,float*b){View_AnglesToAxis(a,b);}
static void MS MLinked(int a,int c){View_Linked(a,c);}
static void MS MLastStandSway(){View_LastStandSway();}
static void MS MVehicle(int c){View_Vehicle(c);}
static void MS MVehicleThirdPerson(int c){View_VehicleThirdPerson(c);}
static void MS MMissile(int c){View_Missile(c);}
static void MS MSpike(){View_Spike();}
static void MS MSub_140621C10(const Byte*p,float*a){View_Sub_140621C10(p,a);}
static void MS MRemoteEyes(int c){View_RemoteEyes(c);}
static void MS MTurret(int c){View_Turret(c);}
static void MS MGunner(int c,Byte*p){View_Gunner(c,p);}
static void MS MFirstPerson(Byte*p){View_FirstPerson(p);}
static void MS MOffsetVehicle(int c,int m){View_OffsetVehicle(c,m);}
static void MS MShake(int c){View_Shake(c);}
static void MS MVelocity(Byte*p){View_Velocity(p);}
static void MS MTween(int c){View_Tween(c);}
static void MS MMissileAngles(int c){View_MissileAngles(c);}
static int MS MUsingVehicleWeapon(const Byte*p){return View_UsingVehicleWeapon(p);}
static U32 MS MWeapon(const Byte*p){return View_Weapon(p);}
static void MS MSub_14017F0B0(Byte*p){View_Sub_14017F0B0(p);}
static Byte* MS MSub_1400E6370(Byte*p,Byte*q,unsigned id){return View_Sub_1400E6370(p,q,id);}
extern "C" __attribute__((ms_abi,visibility("hidden"),noinline)) void MainTagAxisBridge(Byte*p,float*b){View_Sub_1400DE970(p,b);}
extern "C" __attribute__((ms_abi,naked)) void MSub_1400DE970(Byte*,float*){
    __asm__("push %rcx\n\tsub $32,%rsp\n\tcall MainTagAxisBridge\n\tadd $32,%rsp\n\tpop %rcx\n\tret");
}
static void MS MAxisToAngles(const float*a,float*b){View_AxisToAngles(a,b);}
static void MS MThirdPerson(int c){View_ThirdPerson(c);}
static void MS MPerturb(Byte*p){View_Perturb(p);}
static int MS MAtoi(const char*s){return View_Atoi(s);}
static double MS MAtof(const char*s){return View_Atof(s);}
extern "C" {
void *MainViewMock(unsigned i){
    void*m[]={reinterpret_cast<void*>(MUpdateMode),reinterpret_cast<void*>(MCalcFov),reinterpret_cast<void*>(MApplyVision),reinterpret_cast<void*>(MCubemap),reinterpret_cast<void*>(MAnglesToAxis),reinterpret_cast<void*>(MLinked),reinterpret_cast<void*>(MLastStandSway),reinterpret_cast<void*>(MVehicle),reinterpret_cast<void*>(MVehicleThirdPerson),reinterpret_cast<void*>(MMissile),reinterpret_cast<void*>(MSpike),reinterpret_cast<void*>(MSub_140621C10),reinterpret_cast<void*>(MRemoteEyes),reinterpret_cast<void*>(MTurret),reinterpret_cast<void*>(MGunner),reinterpret_cast<void*>(MFirstPerson),reinterpret_cast<void*>(MOffsetVehicle),reinterpret_cast<void*>(MShake),reinterpret_cast<void*>(MVelocity),reinterpret_cast<void*>(MTween),reinterpret_cast<void*>(MMissileAngles),reinterpret_cast<void*>(MUsingVehicleWeapon),reinterpret_cast<void*>(MWeapon),reinterpret_cast<void*>(MSub_14017F0B0),reinterpret_cast<void*>(MSub_1400E6370),reinterpret_cast<void*>(MSub_1400DE970),reinterpret_cast<void*>(MAxisToAngles),reinterpret_cast<void*>(MThirdPerson),reinterpret_cast<void*>(MPerturb),reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MAtof)};
    assert(i<sizeof(m)/sizeof(m[0]));return m[i];
}
unsigned MainViewSize(){return sizeof(Trace);}
void MainViewRun(void*image,U32 input,U32 config,U32 variant,void*out){
    key=input;trace={};mode=int(key%12);
    const unsigned fields[]={0x4d95c,0x4d960,0x646e4,0x4e82c,0x64888,0x6488c,0x64890,0x4db48,
      0x4d8f0,0x4d8f4,0x4d8f8,0x4d8fc,0x646e0,0x4d900,0x4d904,0x4d908,0x4d90c,
      0x4d924,0x4d928,0x4d92c,0x649d0,0x649d4,0x649d8,0x6b694,0x6b698,0x4aec0,
      0x649e8,0x649ec,0x649f0,0x4d934,0x4d938,0x4d93c,0x4d940,0x4d944,0x4d948,0x4d94c,0x4d950,0x4d954,
      0x6b6e4,0x6b6e8,0x6b6ec,0x6b6f0,0x6b6f4,0x6b6f8,0x6b6fc,0x6b700,0x6b704,0x6b708,0x6b70c,0x6b710,0x648e0};
    for(unsigned o:fields)W(state,o,U32(0x42424242));
    View_State=state;View_Static=statics;View_Playback=play;W(state,0x30,static_cast<Byte*>(snap));
    View_SkinPressure=pressure;View_BlurA=blurA;View_ClientMapping=mapping;View_BlurB=blurB;View_MenuBlur=menus+0x24;
    const float fv[]={0,0.05f,0.1f,0.5f,0.9f,0.95f,1.0f,2.0f};
    W(pressure,0,fv[variant&7]);W(blurA,0,1.25f);W(blurB,0,2.75f);
    W(mapping,0,int((variant>>3)&1));W(mapping,4,int((variant>>4)&1));
    W(menus,0,3.25f);W(menus,0x24,4.5f);W(menus,0x48,5.75f);
    View_GlobalBlur=0.75f;View_BlurFlags=(variant&32)?8:0;View_BlurEnabled=(variant>>6)&1;
    View_DemoMode=(key&16)?2:0;View_DemoFlags=(key&32)?2:0;View_PlaybackInited=(key>>6)&1;
    W(play,0xd4c00,int((key&128)?2:0));W(play,0x26a998,Byte((key>>8)&1));
    W(state,0xc,int((key>>9)&1));W(state,0x4808c,int(1000+(variant&127)));
    W(state,0x481a8,U32((key&1024)?0xe:0));
    const int pm[]={0,1,5,6,11,2,3,0};
    W(state,0x480ac,pm[(key>>11)&7]);W(state,0x482ec,int((key>>14)&3));
    W(state,0x6b818,int((key>>16)&1));W(state,0x6b82c,int((key&131072)?17:0));
    W(state,0x480b0,Byte(variant>>8));W(state,0x480b4,Byte((key&262144)?8:0));
    W(state,0x48168,int((variant&128)?100:800));W(state,0x480dc,3.0f);W(state,0x480e0,4.0f);W(state,0x480e4,12.0f);
    W(state,0x83484,Byte((variant>>16)&1));W(snap,0x14,int((variant>>17)&1));
    W(state,0x480a0,int((key>>19)&1));W(state,0x83490,fv[(variant>>18)&7]*20.0f);
    W(state,0x8348c,int((variant&0x200000)?1200:950));
    W(state,0x4852c,Byte((variant&0x400000)?4:0));
    W(state,0x481a4,U32((variant&0x800000)?0x300:0));
    W(state,0x4aec0,int((variant&0x1000000)?1050:970));
    W(state,0x48288,fv[(variant>>25)&7]);
    for(unsigned i=0;i<3;++i){W(state,0x480d0+4*i,float(i+10));W(state,0x482a0+4*i,float(i+30));W(state,0x4aec4+4*i,float(i+1));W(tag,0x10+4*i,float(i+70));}
    for(unsigned i=0;i<9;++i)W(statics,4*i,U32(i*0x1234567u));
    object=(key&(1u<<22))?obj:nullptr;obj[0x79]=0x23;View_TagObject=&object;View_TagId=27;
    const Dvar **slots[]={&View_AdaptiveSkin,&View_OverflowScale,&View_HelicopterCull,&View_SmoothDuration,&View_AdsScale};
    const float values[]={1,2,1,0.25f,1.5f};
    for(unsigned i=0;i<5;++i){
        const unsigned kind=(config>>(2*i))&3;
        *slots[i]=kind?&dvars[i]:nullptr;
        const bool boolean=i==0||i==2;
        W(&dvars[i],0x18,int(kind==1?(boolean?1:2):kind==2?(boolean?8:6):8));
        if(kind==1){if(boolean)W(&dvars[i],0x20,Byte((variant>>(28+i))&1));else W(&dvars[i],0x20,values[i]);}
        else if(kind==2&&!boolean)W(&dvars[i],0x20,int(values[i]));
        else W(&dvars[i],0x20,strings[(config>>(10+3*i))&7]);
    }
    if(image){
        W(image,0xe53a20,View_State);W(image,0xe53b70,View_Static);W(image,0x2653900,View_Playback);
        const unsigned rvas[]={0xe5a650,0x27e6ca8,0xd78610,0xe4ba58,0xe49a20};
        for(unsigned i=0;i<5;++i)W(image,rvas[i],*slots[i]);
        W(image,0x27f1d6c,R<U32>(pressure,0));W(image,0x3bfd290,R<U32>(blurA,0));
        W(image,0x1ef91e4,R<U32>(mapping,0));W(image,0x1ef91e8,R<U32>(mapping,4));
        for(int i=-1;i<=1;++i)W(image,0x43c54fc+i*0x24,R<U32>(View_MenuBlur,i*0x24));
        W(image,0xe4f91c,R<U32>(blurB,0));W(image,0x21214ec,View_GlobalBlur);
        W(image,0xe61794,View_BlurFlags);W(image,0x2cb7dcc,View_BlurEnabled);
        W(image,0x25a3308,View_DemoMode);W(image,0x25a330c,View_DemoFlags);W(image,0x2653908,View_PlaybackInited);
        W(image,0xd76830,View_TagObject);W(image,0x1ef6b1a,View_TagId);
        reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0x16c510)(0);
    }else CalcViewValues(0);
    unsigned i=0;for(unsigned o:fields)trace.fields[i++]=R<U32>(state,o);trace.fields[i++]=obj[0x79];
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
