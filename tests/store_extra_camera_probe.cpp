#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_extra_camera_store.cpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_extra_camera {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x70000],playback[0x26a999];
static Dvar dvars[2];
static const char *strings[]={"0","1","0.5","-1","90","nan","inf","abc"};
static unsigned mode;
struct Event{U kind,a,b,c;};
struct Trace{U count;Event events[16];U output[32];};
static Trace trace;
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<16);trace.events[trace.count++]={k,a,b,c};}
extern "C" {
Byte *Extra_State;
const Dvar *Extra_Fov,*Extra_Aspect;
U32 Extra_ViewportWidth,Extra_ViewportHeight,Extra_AltWidth,Extra_AltHeight;
int Extra_DemoMode;
Byte Extra_DemoFlags,Extra_PlaybackInited;
const Byte *Extra_Playback;
int Extra_GetMode(int c){Push(1,c);return mode;}
void Extra_Update(int c,Byte b){Push(2,c,b);}
void Extra_Special(Byte*p){assert(p==state);Push(3);}
void Extra_Linked(int a,int c){Push(4,a,c);}
void Extra_Mode6A(int c){Push(5,c);}
void Extra_Mode6B(int c){Push(6,c);}
void Extra_Mode7(){Push(7);}
void Extra_AnglesToAxis(const float*a,float*b){
    assert(reinterpret_cast<const Byte*>(a)==state+0x649d0);
    assert(reinterpret_cast<Byte*>(b)==state+0x4d934);
    Push(8,Bits(a[0]),Bits(a[1]),Bits(a[2]));
    for(unsigned i=0;i<9;++i)b[i]=float(i)+a[i%3];
}
float Extra_Tan(float v){Push(9,Bits(v));return std::tan(v);}
double Extra_Atof(const char*s){unsigned i=0;while(i<8&&s!=strings[i])++i;assert(i<8);Push(10,i);return std::atof(s);}
}
static int MS MMode(int c){return Extra_GetMode(c);}
static void MS MUpdate(int c,Byte b){Extra_Update(c,b);}
static void MS MSpecial(Byte*p){Extra_Special(p);}
static void MS MLinked(int a,int c){Extra_Linked(a,c);}
static void MS M6A(int c){Extra_Mode6A(c);}
static void MS M6B(int c){Extra_Mode6B(c);}
static void MS M7(){Extra_Mode7();}
static void MS MAxis(const float*a,float*b){Extra_AnglesToAxis(a,b);}
static float MS MTan(float f){return Extra_Tan(f);}
static double MS MAtof(const char*s){return Extra_Atof(s);}
extern "C" {
void *ExtraMock(unsigned i){
    void *m[]={reinterpret_cast<void*>(MMode),reinterpret_cast<void*>(MUpdate),reinterpret_cast<void*>(MSpecial),
        reinterpret_cast<void*>(MLinked),reinterpret_cast<void*>(M6A),reinterpret_cast<void*>(M6B),reinterpret_cast<void*>(M7),
        reinterpret_cast<void*>(MAxis),reinterpret_cast<void*>(MTan),reinterpret_cast<void*>(MAtof)};
    assert(i<10);return m[i];
}
unsigned ExtraTraceSize(){return sizeof(Trace);}
void ExtraRun(void*image,unsigned key,unsigned config,U32 fov,U32 aspect,void*out){
    trace={};mode=key%10;int client=(key>>4)&1;
    Extra_State=state;Extra_Playback=playback;
    const unsigned outputs[]={0x648e0,0x4e82c,0x4d910,0x4d914,0x4d920,0x4d91c,0x4d8f0,0x4d8f4,0x4d8f8,0x4d8fc,0x646e0,
        0x4d900,0x4d904,0x4d908,0x4d90c,0x4d924,0x4d928,0x4d92c,0x649d0,0x649d4,0x649d8,
        0x4d934,0x4d938,0x4d93c,0x4d940,0x4d944,0x4d948,0x4d94c,0x4d950,0x4d954};
    for(unsigned o:outputs)Write(state,o,U32(0x42424242));
    Write(state,0xc,int((key>>5)&1));Write(state,0x64b00,Byte((key>>6)&1));
    Write(state,0x64afc,int((key>>7)&3));Write(state,0x480ac,int((key&512)?6:0));
    Write(state,0x6b818,int((key>>10)&1));Write(state,0x6b82c,int((key&2048)?17:0));
    Extra_DemoMode=(key&4096)?2:0;Extra_PlaybackInited=Byte((key>>13)&1);Extra_DemoFlags=(key&16384)?2:0;
    Write(playback,0x26a998,Byte((key>>15)&1));
    for(unsigned i=0;i<4;++i)Write(state,0x64aec+4*i,30.0f+10*i);
    for(unsigned i=0;i<3;++i){Write(state,0x480d0+4*i,10.0f+i);Write(state,0x482a0+4*i,30.0f+i);}
    for(unsigned i=0;i<24;++i)Write(state,0x64a7c+4*i,100.0f+i);
    Write(state,0x482b0,3.75f);
    Extra_ViewportWidth=1920;Extra_ViewportHeight=1080;Extra_AltWidth=640;Extra_AltHeight=480;
    if(config&256)Extra_AltWidth=0xffffffffu;
    const Dvar **slots[]={&Extra_Fov,&Extra_Aspect};
    const U32 bits[]={fov,aspect};
    for(unsigned i=0;i<2;++i){
        unsigned kind=(config>>(2*i))&3;
        *slots[i]=kind?&dvars[i]:nullptr;
        Write(&dvars[i],0x18,int(kind==1?2:kind==2?6:8));
        if(kind==1)Write(&dvars[i],0x20,bits[i]);
        else if(kind==2)Write(&dvars[i],0x20,int((bits[i]>>16)&127)-32);
        else Write(&dvars[i],0x20,strings[(config>>(10+3*i))&7]);
    }
    if(image){
        Write(image,0xe53a20,Extra_State);Write(image,0xd78638,Extra_Fov);Write(image,0x27e6e58,Extra_Aspect);
        Write(image,0x3043c38,Extra_ViewportWidth);Write(image,0x3043c3c,Extra_ViewportHeight);
        Write(image,0x3043c40,Extra_AltWidth);Write(image,0x3043c44,Extra_AltHeight);
        Write(image,0x25a3308,Extra_DemoMode);Write(image,0x25a330c,Extra_DemoFlags);
        Write(image,0x2653908,Extra_PlaybackInited);Write(image,0x2653900,Extra_Playback);
        reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0x16ce80)(client);
    }else CalcExtraView(client);
    unsigned i=0;for(unsigned o:outputs)trace.output[i++]=Read<U32>(state,o);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
