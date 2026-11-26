#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_vehicle_offset_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_vehicle_offset {
struct Dvar{Byte bytes[104];};static Dvar dvar;
alignas(16) static Byte state[0x6b800],entities[0x3d8*2],weapon[0x18],props[0xb30],def[0xba8];
static int timeSlot;static float pitchSlot;static unsigned key,randomCount;
struct Trace{U count;U events[12][3];U fields[10];};static Trace trace;
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static void Push(U k,U a=0,U b=0){assert(trace.count<12);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;}
extern "C" {
Byte *VehicleOffset_State,*VehicleOffset_Entities[1];
const Byte *VehicleOffset_Weapons[256],*VehicleOffset_Definitions[256];
const Dvar *VehicleOffset_DisableJitter;
int VehicleOffset_DemoMode,*VehicleOffset_DemoTime;
float *VehicleOffset_LastPitch;
const char *VehicleOffset_ErrorText;
bool VehicleOffset_Suppress(){Push(1);return key&1;}
bool VehicleOffset_ErrorCheck(){Push(2);return key&2;}
void VehicleOffset_Error(int c,const char*s){assert(s==VehicleOffset_ErrorText);Push(3,c);}
int VehicleOffset_Atoi(const char*s){int r=std::atoi(s);Push(4,U32(r));return r;}
int VehicleOffset_UsingVehicleWeapon(const Byte*p){assert(p==state+0x480a8);Push(5);return key&4;}
U32 VehicleOffset_SelectWeapon(const Byte*p,int c){assert(p==state+0x480a8);Push(6,c);return (key&8)?1:256;}
U32 VehicleOffset_TurretWeapon(int c){Push(7,c);return (key&8)?1:0;}
U32 VehicleOffset_DefaultWeapon(const Byte*p){assert(p==state+0x480a8);Push(8);return 1;}
int VehicleOffset_Random(){Push(9,randomCount);return randomCount++?32767:12345;}
}
static bool MS M0(){return VehicleOffset_Suppress();}
static bool MS M1(){return VehicleOffset_ErrorCheck();}
static void MS M2(int c,const char*s){VehicleOffset_Error(c,s);}
static int MS M3(const char*s){return VehicleOffset_Atoi(s);}
static int MS M4(const Byte*p){return VehicleOffset_UsingVehicleWeapon(p);}
static U32 MS M5(const Byte*p,int c){return VehicleOffset_SelectWeapon(p,c);}
static U32 MS M6(int c){return VehicleOffset_TurretWeapon(c);}
static U32 MS M7(const Byte*p){return VehicleOffset_DefaultWeapon(p);}
static int MS M8(){return VehicleOffset_Random();}
extern "C" {
void*VehicleOffsetMock(unsigned i){void*m[]={reinterpret_cast<void*>(M0),reinterpret_cast<void*>(M1),reinterpret_cast<void*>(M2),reinterpret_cast<void*>(M3),reinterpret_cast<void*>(M4),reinterpret_cast<void*>(M5),reinterpret_cast<void*>(M6),reinterpret_cast<void*>(M7),reinterpret_cast<void*>(M8)};assert(i<9);return m[i];}
unsigned VehicleOffsetSize(){return sizeof(Trace);}
void VehicleOffsetRun(void*image,void*mapped,unsigned seed,U32 pitch,U32 prev,int now,int old,void*out){
    key=seed;randomCount=0;trace={};VehicleOffset_State=state;VehicleOffset_Entities[0]=entities;
    VehicleOffset_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    VehicleOffset_Weapons[1]=weapon;VehicleOffset_Definitions[1]=def;
    W(weapon,0x10,(key&16)?static_cast<Byte*>(props):nullptr);W(props,0xb2c,7.0f);W(props,0xb28,3.0f);
    W(state,0x4851c,std::int16_t(1));W(state,0x48520,int((key>>5)&7));
    W(state,0x481a4,U32((key&256)?0x300:0));
    Byte*e=entities+0x3d8;W(e,0x1f8,Byte((key&512)?0x40:0));W(e,0x284,std::int16_t((key&1024)?0x7fff:0));W(e,0x286,Byte(1));
    W(state,0x649d0,pitch);W(state,0x649d4,40.0f);W(state,0x4808c,now);
    for(unsigned i=0;i<6;++i)W(state,0x6b69c+4*i,0x42424242u);
    W(def,8,static_cast<unsigned short>((key>>11)&3));W(def,0x28,static_cast<unsigned short>((key&8192)?4:0));W(def,0xba4,int((key>>14)&1));
    unsigned kind=(key>>15)&7;VehicleOffset_DisableJitter=kind?&dvar:nullptr;
    int t=kind==1?2:kind==2?6:kind==3?7:kind==4?1:8;
    W(&dvar,0x18,t);if(t==2)W(&dvar,0x20,(key&262144)?1.0f:0.0f);else if(t==6||t==7)W(&dvar,0x20,int((key>>18)&1));else if(t==1)W(&dvar,0x20,Byte((key>>18)&1));else W(&dvar,0x20,(key&262144)?"1":"0");
    VehicleOffset_DemoMode=(key&524288)?2:0;VehicleOffset_DemoTime=&timeSlot;VehicleOffset_LastPitch=&pitchSlot;
    timeSlot=old;pitchSlot=F(prev);const int mode=(key&1048576)?2:3;
    if(image){
        W(image,0xe53a20,VehicleOffset_State);W(image,0xe4f8e0,VehicleOffset_Entities[0]);W(image,0x265d3b8,VehicleOffset_Weapons[1]);W(image,0x265d0b8,VehicleOffset_Definitions[1]);W(image,0x1f04888,VehicleOffset_DisableJitter);W(image,0x25a3308,VehicleOffset_DemoMode);
        W(image,0x449ee5c,old);W(image,0x449e700,prev);
        reinterpret_cast<void(MS*)(int,int)>(static_cast<Byte*>(image)+0xc66d0)(0,mode);
        timeSlot=R<int>(image,0x449ee5c);pitchSlot=R<float>(image,0x449e700);
    }else OffsetVehicleView(0,mode);
    trace.fields[0]=R<U32>(state,0x649d0);trace.fields[1]=R<U32>(state,0x649d4);trace.fields[2]=U32(timeSlot);trace.fields[3]=Bits(pitchSlot);
    for(unsigned i=0;i<6;++i)trace.fields[4+i]=R<U32>(state,0x6b69c+4*i);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
