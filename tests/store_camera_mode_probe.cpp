#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_mode_store.cpp"
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_mode {
static Byte state[0x6b900],playback[0xd4c04],entity[0x3d8*1024],def[0x300],clientState[0x2ca8],snap[2][0x1b4];
static unsigned key;
static const char sound[]="vehicle_sound";
struct Trace{U count;U events[12][4];U fields[12];};
static Trace trace;
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<12);auto *p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;}
extern "C" {
Byte *Mode_State,*Mode_EntityBase[1],*Mode_ClientState;
int Mode_Demo,Mode_ClientCount;
Byte Mode_PlaybackInited;
const Byte *Mode_Playback,*Mode_VehicleDefs[256];
const char *Mode_ErrorText;
bool Mode_ErrorCheck(){Push(1);return (key>>22)&1;}
void Mode_Error(int c,const char*s){assert(s==Mode_ErrorText);Push(2,c);}
int Mode_Missile(int c){Push(3,c);return (key>>23)&1;}
void Mode_Transition(int c,int a,int b){Push(4,c,a,b);}
void Mode_Sound(const char*s,int v){assert(!std::strcmp(s,"no_gfutz")||s==sound);Push(5,s==sound,v);}
void Mode_UpdateVehicle(int c){Push(6,c);}
}
static bool MS MCheck(){return Mode_ErrorCheck();}
static void MS MError(int c,const char*s){Mode_Error(c,s);}
static int MS MMissile(int c){return Mode_Missile(c);}
static void MS MTransition(int c,int a,int b){Mode_Transition(c,a,b);}
static void MS MSound(const char*s,int v){Mode_Sound(s,v);}
static void MS MUpdate(int c){Mode_UpdateVehicle(c);}
extern "C" {
void *ModeMock(unsigned i){void *m[]={reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MMissile),reinterpret_cast<void*>(MTransition),reinterpret_cast<void*>(MSound),reinterpret_cast<void*>(MUpdate)};assert(i<6);return m[i];}
unsigned ModeSize(){return sizeof(Trace);}
void ModeRun(void*image,void*mapped,U32 seed,U32 config,void*out){
    trace={};key=seed;Mode_State=state;Mode_ClientState=clientState;Mode_ClientCount=1;
    Mode_EntityBase[0]=entity;Mode_Playback=playback;Mode_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    Mode_Demo=(key&1)?2:0;Mode_PlaybackInited=(key>>1)&1;W(playback,0xd4c00,int((key&4)?2:0));
    W(state,0x481a4,U32((key&8)?0x4000:(key&16)?0x300:0));
    W(state,0x481a8,U32(((key&32)?0x40000:0)|((key&64)?0x10000000:0)));
    const int n=(key&128)?0x3ff:1;
    W(state,0x4851c,std::int16_t(n));W(entity,n*0x3d8+0x286,Byte(1));
    Mode_VehicleDefs[1]=def;W(def,8,static_cast<unsigned short>(config&7));W(def,0x28,static_cast<unsigned short>((config>>3)&1));W(def,0x30,int((config>>4)&1));
    const char *s=(config&32)?sound:(config&64)?"":nullptr;W(def,0x2f0,s);
    W(state,0x48520,int((key>>8)&7));W(state,0x480a0,int((key>>11)&1));W(state,0x480ac,int((key>>12)&1));
    W(state,0x64a30,int((key>>13)&7));W(state,0x64a34,int((key>>16)&1));W(state,0x64a38,int((config>>7)&7));
    W(state,0x64b04,Byte((key>>17)&1));W(state,0x64b05,Byte(0x42));W(state,0x6a3e0,int((key>>18)&1));
    W(state,0x48248,int((key>>19)&1));W(state,0x64a08,int((key>>20)&1));
    W(state,0x6b818,int((key>>21)&1));W(state,0x6b81c,int((config>>10)&1));
    W(state,0x28,static_cast<Byte*>(snap[0]));W(state,0x30,(config&2048)?static_cast<Byte*>(snap[1]):nullptr);
    W(snap[0],0x1b0,int((config>>12)&1));W(snap[1],0x1b0,int((config>>13)&1));
    W(state,0x480c0,Byte((config&16384)?2:0));W(state,0x48140,(config&32768)?-35.0f:35.0f);W(clientState,0x2ca4,20.0f);
    if(image){
        W(image,0xe53a20,Mode_State);W(image,0xe4f8e0,Mode_EntityBase[0]);W(image,0x265d0b8,Mode_VehicleDefs[1]);
        W(image,0x2653900,Mode_Playback);W(image,0x25a3308,Mode_Demo);W(image,0x2653908,Mode_PlaybackInited);
        W(image,0xe5ca9c,Mode_ClientCount);W(image,0xe61bf8,Mode_ClientState);
        trace.fields[0]=reinterpret_cast<int(MS*)(int)>(static_cast<Byte*>(image)+0xc0e20)(0);
    }else trace.fields[0]=UpdateCameraMode(0);
    const unsigned offsets[]={0x64a30,0x64a34,0x64a38,0x64b04,0x64a08,0x6a3e0};
    unsigned i=1;for(unsigned o:offsets)trace.fields[i++]=o==0x64b04?R<Byte>(state,o):R<U32>(state,o);
    trace.fields[i++]=R<Byte>(state,0x64b05);trace.fields[i++]=R<U32>(clientState,0x2ca4);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
