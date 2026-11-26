#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_third_person_store.cpp"
#include <cmath>
#include <cstdlib>
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_third_camera {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x6b900],entities[0x3d8*24],models[0x90*4],tags[2][32],play[0xd4c04];
static std::int16_t map[0x701];
static Dvar dvars[4];
static const char*strings[]={"0","1","2","45","90","-20","0.5","abc"};
static unsigned key;static unsigned tagCalls;
struct Trace{U count;U events[24][6];U outputs[12];};
static Trace trace;
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0,U e=0){assert(trace.count<24);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;p[5]=e;}
extern "C" {
Byte *Third_State,*Third_Entities[1],*Third_Models;
const std::int16_t *Third_ModelMap;
unsigned short Third_PlayerTag=11,Third_DeathTag=12;
int Third_DemoMode;Byte Third_PlaybackInited;const Byte *Third_Playback;
const Dvar *Third_Mode,*Third_Angle,*Third_Range,*Third_AboveWater;
const char*Third_ErrorText;
Byte*Third_Tag(Byte*p,Byte*m,unsigned id){Push(1,p==state+0x4aab8,((m-Third_Models)/0x90),id);unsigned i=tagCalls++;return ((key>>(8+i))&1)?tags[i]:nullptr;}
void Third_TagAxis(Byte*p,float*a){assert(p==tags[0]);Push(2);for(unsigned i=0;i<9;++i)a[i]=0;}
bool Third_ErrorCheck(){Push(3);return (key>>10)&1;}
void Third_Error(int c,const char*s){assert(s==Third_ErrorText);Push(4,c);}
void Third_Chase(int c){Push(5,c);}
int Third_Atoi(const char*s){Push(6,std::atoi(s));return std::atoi(s);}
double Third_Atof(const char*s){Push(7,Bits(float(std::atof(s))));return std::atof(s);}
__m128 Third_SinCos(float a,float b){Push(8,Bits(a),Bits(b));return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
void Third_Trace(Byte*p,float*a,const float*b,int mask,float*out,Byte q,Byte r){
    assert(p==state&&a==out&&a==reinterpret_cast<float*>(state+0x4d924));Push(9,Bits(b[0]),Bits(b[1]),Bits(b[2]),mask,q|(r<<8));
    for(unsigned i=0;i<3;++i)out[i]=b[i]+((key&2048)?float(i+1):0.0f);
}
float Third_WaterHeight(float*p,float a,float b){assert(p==reinterpret_cast<float*>(state+0x4d924));Push(10,Bits(a),Bits(b));return (key&4096)?1000.0f:-200000.0f;}
float Third_Atan2(float y,float x){Push(11,Bits(y),Bits(x));return std::atan2(y,x);}
void Third_AnglesToAxis(const float*a,float*b){assert(b==reinterpret_cast<float*>(state+0x4d934));Push(12,Bits(a[0]),Bits(a[1]),Bits(a[2]));for(unsigned i=0;i<9;++i)b[i]=a[i%3]+float(i);}
}
static Byte* MS MTag(Byte*p,Byte*m,unsigned id){return Third_Tag(p,m,id);}
extern "C" __attribute__((ms_abi,visibility("hidden"),noinline)) void ThirdTagAxisBridge(Byte*p,float*b){Third_TagAxis(p,b);}
extern "C" __attribute__((ms_abi,naked)) void MTagAxis(Byte*,float*){__asm__("push %rcx\n\tsub $32,%rsp\n\tcall ThirdTagAxisBridge\n\tadd $32,%rsp\n\tpop %rcx\n\tret");}
static bool MS MCheck(){return Third_ErrorCheck();}
static void MS MError(int c,const char*s){Third_Error(c,s);}
static void MS MChase(int c){Third_Chase(c);}
static int MS MAtoi(const char*s){return Third_Atoi(s);}
static double MS MAtof(const char*s){return Third_Atof(s);}
static __m128 MS MTrig(float a,float b){return Third_SinCos(a,b);}
static void MS MTrace(Byte*p,float*a,const float*b,int m,float*out,Byte q,Byte r){Third_Trace(p,a,b,m,out,q,r);}
static float MS MWater(float*p,float a,float b){return Third_WaterHeight(p,a,b);}
static float MS MAtan(float a,float b){return Third_Atan2(a,b);}
static void MS MAxis(const float*a,float*b){Third_AnglesToAxis(a,b);}
extern "C" {
void*ThirdMock(unsigned i){void*m[]={reinterpret_cast<void*>(MTag),reinterpret_cast<void*>(MTagAxis),reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MChase),reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MTrig),reinterpret_cast<void*>(MTrace),reinterpret_cast<void*>(MWater),reinterpret_cast<void*>(MAtan),reinterpret_cast<void*>(MAxis)};assert(i<12);return m[i];}
unsigned ThirdSize(){return sizeof(Trace);}
void ThirdRun(void*image,void*mapped,unsigned seed,unsigned config,void*out){
    key=seed;tagCalls=0;trace={};Third_State=state;Third_Entities[0]=entities;Third_Playback=play;
    Third_Models=image?static_cast<Byte*>(image)+0x3f85390:models;Third_ModelMap=map;
    Third_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    W(state,0x4852c,Byte((key&1)?4:0));W(state,0x481a4,U32((key&2)?0x300:0));
    W(state,0x48248,1);map[1]=(key&4)?1:0;map[2]=(key&8)?2:0;
    W(state,0x480ac,int((key&16)?10:0));W(state,0x4819c,int((key&32)?1:4));
    Byte*ent=entities+19*0x3d8;W(ent,0x3cc,Byte((key&64)?2:0));W(ent,0x1f4,2);W(ent,0x2ce,static_cast<unsigned short>((key&128)?0x3ff:1));
    W(state,0x480c0,Byte((key&8192)?2:0));Third_DemoMode=(key&16384)?2:0;Third_PlaybackInited=(key>>15)&1;W(play,0xd4c00,int((key>>16)&1));
    W(state,0x482e4,135);
    for(unsigned i=0;i<3;++i){W(state,0x4d924+4*i,float(10+i));W(state,0x480d0+4*i,float(20+i));
        W(state,0x649d0+4*i,float(int((config>>(i*4))&15)-8)*15.0f);W(tags[0],0x10+4*i,float(30+i));W(tags[1],0x10+4*i,float((key&131072)?60+i:21+i));}
    for(unsigned i=0;i<9;++i)W(state,0x4d934+4*i,U32(0x42424242));
    const Dvar**slots[]={&Third_Mode,&Third_Angle,&Third_Range,&Third_AboveWater};const float values[]={2,45,100,10};
    for(unsigned i=0;i<4;++i){unsigned kind=(config>>(12+3*i))&7;*slots[i]=kind?&dvars[i]:nullptr;int t=kind==1?2:kind==2?6:kind==3&&i==0?7:kind==4&&i==0?1:8;
        W(&dvars[i],0x18,t);if(t==2)W(&dvars[i],0x20,values[i]);else if(t==6||t==7)W(&dvars[i],0x20,int(values[i]));else if(t==1)W(&dvars[i],0x20,Byte(1));else W(&dvars[i],0x20,strings[(key>>(18+2*i))&7]);}
    if(image){
        W(image,0xe53a20,Third_State);W(image,0xe4f8e0,Third_Entities[0]);W(image,0x3fcd392,map[1]);W(image,0x3fcd394,map[2]);
        W(image,0x1ef6b8a,Third_PlayerTag);W(image,0x1ef6b84,Third_DeathTag);W(image,0x25a3308,Third_DemoMode);W(image,0x2653908,Third_PlaybackInited);W(image,0x2653900,Third_Playback);
        const unsigned rvas[]={0xe4bae8,0xe4ba78,0xe4ba98,0xe4f848};for(unsigned i=0;i<4;++i)W(image,rvas[i],*slots[i]);
        reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0x16a940)(0);
    }else OffsetThirdPersonView(0);
    for(unsigned i=0;i<3;++i)trace.outputs[i]=R<U32>(state,0x4d924+4*i);
    for(unsigned i=0;i<9;++i)trace.outputs[3+i]=R<U32>(state,0x4d934+4*i);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
