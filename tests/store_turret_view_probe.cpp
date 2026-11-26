#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_turret_view_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_turret_view {
alignas(16) static Byte state[0x65000],entity[0x3d8*2],models[0x90*2],tag[32],weapon[0x18],props[0xb30];
static std::int16_t map[0x701];static int timeSlot;static float offsets[3],aux[3],history[3];
static unsigned key,randomCount;
struct Trace{U count;U calls[10][8];U fields[16];};static Trace trace;
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0,U e=0,U f=0,U g=0){
    assert(trace.count<10);auto*p=trace.calls[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;p[5]=e;p[6]=f;p[7]=g;
}
extern "C" {
Byte *TurretView_State,*TurretView_Entities[1],*TurretView_Models;
const std::int16_t *TurretView_ModelMap;
const Byte *TurretView_Weapons[256];
unsigned short TurretView_PlayerTag=19;
int *TurretView_LastTime;
float *TurretView_Offset,*TurretView_Aux,*TurretView_PreviousAngles;
const char *TurretView_ErrorText,*TurretView_TagErrorText;
bool TurretView_ErrorCheck(){Push(1);return key&1;}
void TurretView_Error(int c,const char*s){assert(s==TurretView_ErrorText||s==TurretView_TagErrorText);Push(2,c,s==TurretView_TagErrorText);}
const Byte*TurretView_Tag(Byte*e,Byte*m,unsigned n){assert(e==entity+0x3d8&&m==TurretView_Models+0x90);Push(3,n);return (key&2)?tag:nullptr;}
int TurretView_Random(){Push(4,randomCount);return randomCount++?32767:12345;}
void TurretView_ComputeOffset(const float*a,U32 w,float*h,float*o,float*x,float k,int dt){
    assert(a==reinterpret_cast<float*>(state+0x482a0)&&h==TurretView_PreviousAngles&&o==TurretView_Offset&&x==TurretView_Aux);
    Push(5,w,Bits(k),U32(dt),Bits(h[0]),Bits(h[1]),Bits(h[2]));
    for(unsigned i=0;i<3;++i){h[i]+=a[i];o[i]=float(i+1)*0.25f;x[i]+=float(i);}
}
void TurretView_AnglesToAxis(const float*a,float*b){assert(a==reinterpret_cast<float*>(state+0x649d0));Push(6,Bits(a[0]),Bits(a[1]),Bits(a[2]));for(unsigned i=0;i<9;++i)b[i]=float(i+1)*0.125f;}
}
static bool MS MCheck(){return TurretView_ErrorCheck();}
static void MS MError(int c,const char*s){TurretView_Error(c,s);}
static const Byte* MS MTag(Byte*e,Byte*m,unsigned n){return TurretView_Tag(e,m,n);}
static int MS MRandom(){return TurretView_Random();}
static void MS MOffset(const float*a,U32 w,float*h,float*o,float*x,float k,int dt){TurretView_ComputeOffset(a,w,h,o,x,k,dt);}
static void MS MAxis(const float*a,float*b){TurretView_AnglesToAxis(a,b);}
extern "C" {
void*TurretViewMock(unsigned i){void*m[]={reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MTag),reinterpret_cast<void*>(MRandom),reinterpret_cast<void*>(MOffset),reinterpret_cast<void*>(MAxis)};assert(i<6);return m[i];}
unsigned TurretViewSize(){return sizeof(Trace);}
void TurretViewRun(void*image,void*mapped,unsigned seed,int now,int old,void*out){
    key=seed;randomCount=0;trace={};TurretView_State=state;TurretView_Entities[0]=entity;
    TurretView_ModelMap=map;TurretView_Models=image?static_cast<Byte*>(image)+0x3f85390:models;TurretView_Weapons[1]=weapon;
    TurretView_ErrorText=static_cast<char*>(mapped)+0x8e2000;TurretView_TagErrorText=static_cast<char*>(mapped)+0x93c250;
    TurretView_LastTime=&timeSlot;TurretView_Offset=offsets;TurretView_Aux=aux;TurretView_PreviousAngles=history;
    timeSlot=old;
    W(state,0x481a4,U32((key&4)?0x300:0));W(state,0x4851c,std::int16_t(1));W(state,0x480a0,int((key>>3)&1));W(state,0x48518,int((key&16)?2:0));W(state,0x4808c,now);
    W(entity+0x3d8,0x1f4,1);W(entity+0x3d8,0x2d8,0x123401u);map[1]=(key&32)?1:0;W(weapon,0x10,static_cast<const Byte*>(props));
    W(props,0xb2c,3.5f);W(props,0xb28,5.25f);
    for(unsigned i=0;i<3;++i){
        W(state,0x4d924+4*i,float(100+i));W(state,0x649d0+4*i,float(10+i));W(state,0x482a0+4*i,float(20+i));
        W(tag,0x10+4*i,float(30+i));W(props,0x4a8+4*i,float(i+1)*0.5f);
        history[i]=float(40+i);offsets[i]=float(50+i);aux[i]=float(60+i);
    }
    if(image){
        W(image,0xe53a20,TurretView_State);W(image,0xe4f8e0,TurretView_Entities[0]);W(image,0x3fcd392,map[1]);W(image,0x265d3b8,TurretView_Weapons[1]);W(image,0x1ef6b18,TurretView_PlayerTag);
        W(image,0x44a059c,old);std::memcpy(static_cast<Byte*>(image)+0x44a0580,offsets,12);std::memcpy(static_cast<Byte*>(image)+0x44a0590,aux,12);std::memcpy(static_cast<Byte*>(image)+0x44a05a0,history,12);
        TurretView_Offset=reinterpret_cast<float*>(static_cast<Byte*>(image)+0x44a0580);TurretView_Aux=reinterpret_cast<float*>(static_cast<Byte*>(image)+0x44a0590);TurretView_PreviousAngles=reinterpret_cast<float*>(static_cast<Byte*>(image)+0x44a05a0);
        reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0x16bfb0)(0);
        timeSlot=R<int>(image,0x44a059c);std::memcpy(offsets,TurretView_Offset,12);std::memcpy(aux,TurretView_Aux,12);std::memcpy(history,TurretView_PreviousAngles,12);
    }else CalcTurretViewValues(0);
    for(unsigned i=0;i<3;++i){trace.fields[i]=R<U32>(state,0x4d924+4*i);trace.fields[i+3]=R<U32>(state,0x649d0+4*i);trace.fields[i+6]=Bits(history[i]);trace.fields[i+9]=Bits(offsets[i]);trace.fields[i+12]=Bits(aux[i]);}
    trace.fields[15]=U32(timeSlot);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
