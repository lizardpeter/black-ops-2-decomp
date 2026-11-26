#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_view_fov_store.cpp"
#include <cassert>
#include <cstdlib>
#include <limits>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera {
struct Dvar {Byte data[104];};
static Byte state[0x70000],snapshot[0x200],aux[0xd4c04],entity[0x3d8*3],def[0x280],entityDef[0x80],attachment[3][0x48],sentinel[0x48];
static const Byte *attachmentTable[64];
static Dvar dvars[6];
static const char *strings[]={"0","1","0.75","65","10","-2","nan","inf","abc"};
static unsigned seed,weaponCalls;
struct Event {U kind,a,b,c,d;};
struct Trace {U count;Event events[24];U result;};
static Trace trace;
template<class T>static void Write(void *p,unsigned off,T v) {std::memcpy(static_cast<Byte *>(p)+off,&v,sizeof(v));}
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0){assert(trace.count<24);trace.events[trace.count++]={k,a,b,c,d};}
extern "C" {
Byte *Camera_State;
const Dvar *Camera_Fov,*Camera_OverrideFov,*Camera_DefaultFov,*Camera_MinFov,*Camera_Scale,*Camera_EntityOverride;
const Byte *Camera_WeaponDefs[256],*Camera_AttachmentSentinel,*Camera_EntityDefs[256];
Byte *Camera_EntityBase[2];
int Camera_Mode;
Byte Camera_ModeFlags,Camera_AuxEnabled;
const Byte *Camera_AuxState;
const char *Camera_ErrorText;
int Camera_HasWeaponState(const Byte *p){assert(p==state+0x480a8);Push(1);return (seed>>0)&1;}
U32 Camera_SelectWeapon(const Byte *p,int c){assert(p==state+0x480a8);Push(2,c);return (seed&2)?0x000c0201:0x100;}
U32 Camera_SelectOtherWeapon(int c){Push(3,c);return (seed&4)?0x000c0201:0;}
U32 Camera_DefaultWeapon(const Byte *p){assert(p==state+0x480a8);Push(4);return (seed&8)?0:0x001c0201;}
int Camera_OverrideActive(int c){Push(5,c);return (seed>>4)&1;}
float Camera_WeaponFov(void *,const Byte *p,U32 w,float f,Byte alt){assert(p==state+0x480a8);Push(6,w,Bits(f),alt,weaponCalls++);return f+7.5f;}
float Camera_AdjustFov(int c,const Byte *p,float f){assert(p==state+0x480a8);Push(7,c,Bits(f));return f-2.25f;}
bool Camera_ErrorCheck(){Push(8);return (seed>>5)&1;}
void Camera_Error(int code,const char *s){assert(s==Camera_ErrorText);Push(9,code);}
double Camera_Atof(const char *s){unsigned i=0;while(i<9&&s!=strings[i])++i;assert(i<9);Push(10,i);return std::atof(s);}
int Camera_Atoi(const char *s){unsigned i=0;while(i<9&&s!=strings[i])++i;assert(i<9);Push(11,i);return std::atoi(s);}
}
static int MS MHas(const Byte*p){return Camera_HasWeaponState(p);}
static U32 MS MSelect(const Byte*p,int c){return Camera_SelectWeapon(p,c);}
static U32 MS MOther(int c){return Camera_SelectOtherWeapon(c);}
static U32 MS MDefault(const Byte*p){return Camera_DefaultWeapon(p);}
static int MS MOverride(int c){return Camera_OverrideActive(c);}
static float MS MWeapon(void*a,const Byte*p,U32 w,float f,Byte alt){return Camera_WeaponFov(a,p,w,f,alt);}
static float MS MAdjust(int c,const Byte*p,float f){return Camera_AdjustFov(c,p,f);}
static bool MS MErrorCheck(){return Camera_ErrorCheck();}
static void MS MError(int c,const char*s){Camera_Error(c,s);}
static double MS MAtof(const char*s){return Camera_Atof(s);}
static int MS MAtoi(const char*s){return Camera_Atoi(s);}
extern "C" {
void *CameraMock(unsigned i){
    void *m[]={reinterpret_cast<void*>(MHas),reinterpret_cast<void*>(MSelect),reinterpret_cast<void*>(MOther),
      reinterpret_cast<void*>(MDefault),reinterpret_cast<void*>(MOverride),reinterpret_cast<void*>(MWeapon),
      reinterpret_cast<void*>(MAdjust),reinterpret_cast<void*>(MErrorCheck),reinterpret_cast<void*>(MError),
      reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MAtoi)};
    assert(i<11);return m[i];
}
unsigned CameraTraceSize(){return sizeof(Trace);}
void CameraRun(void *image,void *mapped,unsigned key,unsigned flags,unsigned config,void *out) {
    seed=key;weaponCalls=0;trace={};
    const int client=(key>>6)&1;
    const Byte alt=Byte(key>>8);
    Camera_State=state;
    Write(state,0,0);
    Write(state,0x30,(flags&1)?snapshot:static_cast<Byte*>(nullptr));
    Write(state,0x481a4,U32(((flags&2)?0x300:0)|((flags&4)?0x4000:0)));
    Write(state,0x481a8,U32((flags&8)?0x10000000:0));
    Write(state,0x4852c,Byte((flags&16)?4:0));
    Write(state,0x48288,(flags&32)?1.0f:0.0f);
    Write(state,0x480a0,int((flags&64)?1:0));
    Write(state,0x480c0,Byte((flags&128)?2:0));
    Write(state,0x6b9b4,int((flags&256)?9:0));
    Write(snapshot,0x10c,U32((flags&512)?0x4000:0));
    Write(state,0x4851c,std::int16_t(1));
    Camera_Mode=(flags&1024)?2:0;
    Camera_ModeFlags=(flags&2048)?4:0;
    Camera_AuxEnabled=(flags&4096)?1:0;
    Camera_AuxState=aux;Write(aux,0xd4c00,int(config%4));
    Camera_WeaponDefs[1]=def;
    Write(def,0x30,static_cast<const Byte *const *>(attachmentTable));
    Write(def,0x270,40.0f);
    Camera_AttachmentSentinel=sentinel;Write(sentinel,0x40,99.0f);
    attachmentTable[2]=(config&4)?nullptr:attachment[0];
    attachmentTable[48]=(config&8)?nullptr:attachment[1];
    attachmentTable[1]=(config&16)?nullptr:attachment[2];
    Write(attachment[0],0x40,55.0f);Write(attachment[1],0x40,(config&32)?99.0f:35.0f);
    Write(attachment[2],0x40,25.0f);
    Camera_EntityBase[client]=(flags&8192)?nullptr:entity;
    // An index of zero avoids creating a non-null invalid pointer from a null base.
    if(flags&8192)Write(state,0x4851c,std::int16_t(0));
    Write(entity+0x3d8,0x3cc,Byte((flags&16384)?2:0));
    Write(entity+0x3d8,0x286,Byte(1));
    Camera_EntityDefs[1]=entityDef;Write(entityDef,0x74,(config&64)?5.0f:80.0f);
    const Dvar **globals[]={&Camera_Fov,&Camera_OverrideFov,&Camera_DefaultFov,&Camera_MinFov,&Camera_Scale,&Camera_EntityOverride};
    const unsigned rvas[]={0xe4f8e8,0xe5a670,0x2161688,0xd78648,0xd78658,0x265caf8};
    const float values[]={65,30,90,10,(config&128)?0.2f:1.25f,1};
    unsigned v=config>>8;
    for(unsigned i=0;i<6;++i) {
        const unsigned kind=(v>>(i*3))&7;
        *globals[i]=kind?&dvars[i]:nullptr;
        if(i==5) {
            Write(dvars+i,0x18,int(kind==1?1:8));
            if(kind==1)Write(dvars+i,0x20,Byte(config&256?0:1));
            else Write(dvars+i,0x20,strings[kind%9]);
        }else {
            Write(dvars+i,0x18,int(kind==1?2:kind==2?6:8));
            if(kind==1)Write(dvars+i,0x20,values[i]);
            else if(kind==2)Write(dvars+i,0x20,int(values[i]));
            else Write(dvars+i,0x20,strings[kind]);
        }
        if(image)Write(image,rvas[i],*globals[i]);
    }
    Camera_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    if(image){
        Write(image,0xe53a20,Camera_State);
        Write(image,0x25a3308,Camera_Mode);Write(image,0x25a330c,Camera_ModeFlags);
        Write(image,0x2653908,Camera_AuxEnabled);Write(image,0x2653900,Camera_AuxState);
        Write(image,0x265d3b8,Camera_WeaponDefs[1]);Write(image,0x265dbb0,Camera_AttachmentSentinel);
        Write(image,0x265d0b8,Camera_EntityDefs[1]);Write(image,0xe4f8e0+8*client,Camera_EntityBase[client]);
    }
    // The original entity-base array overlaps the fov slot at client 1.
    // Valid tests use local client 0; client 1 needs independent layout recovery.
    assert(client==0);
    const float result=image?reinterpret_cast<float(MS *)(int,Byte)>(static_cast<Byte*>(image)+0xc1b90)(client,alt):CalculateFov(client,alt);
    trace.result=Bits(result);std::memcpy(out,&trace,sizeof(trace));
}
}
}
