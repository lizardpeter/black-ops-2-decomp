#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_remote_fallback_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_remote_fallback {
struct Dvar{Byte bytes[104];};static Dvar dvar;
alignas(16) static Byte state[0x6a430],ents[0x3d8*2],models[0x90*2],tag[32],cl[0x44ac8];
static std::int16_t map[0x701];static int offsets[2]={0,4};static const char names[]="abc\0xyz";
static unsigned key;
struct Trace{U count;U events[16][5];U fields[12];};static Trace trace;
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0){assert(trace.count<16);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
extern "C" {
Byte *Remote_State,*Remote_Entities[1],*Remote_Models;
const std::int16_t *Remote_ModelMap;const int *Remote_StringOffsets;const char *Remote_StringBase;
unsigned short Remote_FallbackTag=99;int Remote_ClientCount;const Byte *Remote_ClientState;const Dvar *Remote_Cull;
const char *Remote_ErrorText,*Remote_CommandError;
bool Remote_ErrorCheck(){Push(1);return key&1;}
void Remote_Error(int c,const char*s){assert(s==Remote_ErrorText);Push(2,c);}
void Remote_ErrorCommand(int c,const char*s,int a,int b){assert(s==Remote_CommandError);Push(3,c,U32(a),U32(b));}
unsigned Remote_FindString(const char*s,unsigned n){assert(n==4);Push(4,s[0],n);return (key&2)?11:0;}
const Byte*Remote_Tag(Byte*e,Byte*m,unsigned n){assert(e==ents+0x3d8&&m==Remote_Models+0x90);Push(5,n);return (n==11?(key&4):(key&8))?tag:nullptr;}
void Remote_TagAxis(const Byte*t,float*a){assert(t==tag);Push(6);for(unsigned i=0;i<9;++i)a[i]=float(i);}
void Remote_AxisToAngles(const float*a,float*b){assert(b==reinterpret_cast<float*>(state+0x6a41c));Push(7,Bits(a[0]),Bits(a[4]),Bits(a[8]));for(unsigned i=0;i<3;++i)b[i]=float(i+10);}
void Remote_UpdateInput(float*a,const Byte*cmd,int ms){assert(a==reinterpret_cast<float*>(state+0x6a41c));Push(8,U32(ms),R<U32>(cmd,0),R<U32>(cmd,0x38));for(unsigned i=0;i<3;++i)a[i]+=float(i+1);}
void Remote_Shake(Byte*c,const Byte*e){assert(c==state&&e==ents+0x3d8);Push(9);W(c,0x649d8,R<float>(c,0x649d8)+2.0f);}
int Remote_Atoi(const char*s){Push(10);return std::atoi(s);}
}
static bool MS MCheck(){return Remote_ErrorCheck();}
static void MS MError(int c,const char*s,int a,int b){if(s==Remote_CommandError)Remote_ErrorCommand(c,s,a,b);else Remote_Error(c,s);}
static unsigned MS MFind(const char*s,unsigned n){return Remote_FindString(s,n);}
static const Byte* MS MTag(Byte*e,Byte*m,unsigned n){return Remote_Tag(e,m,n);}
extern "C" __attribute__((ms_abi,noinline,visibility("hidden"))) void RemoteAxisBridge(const Byte*t,float*a){Remote_TagAxis(t,a);}
// R8 retains the tag pointer across the original LTCG callee.
extern "C" __attribute__((ms_abi,naked)) void MTagAxis(const Byte*,float*){__asm__("push %r8\n\tsub $32,%rsp\n\tcall RemoteAxisBridge\n\tadd $32,%rsp\n\tpop %r8\n\tret");}
static void MS MAngles(const float*a,float*b){Remote_AxisToAngles(a,b);}
static void MS MInput(float*a,const Byte*c,int ms){Remote_UpdateInput(a,c,ms);}
static void MS MShake(Byte*c,const Byte*e){Remote_Shake(c,e);}
static int MS MAtoi(const char*s){return Remote_Atoi(s);}
extern "C" {
void*RemoteMock(unsigned i){void*m[]={reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MFind),reinterpret_cast<void*>(MTag),reinterpret_cast<void*>(MTagAxis),reinterpret_cast<void*>(MAngles),reinterpret_cast<void*>(MInput),reinterpret_cast<void*>(MShake),reinterpret_cast<void*>(MAtoi)};assert(i<9);return m[i];}
unsigned RemoteSize(){return sizeof(Trace);}
void RemoteRun(void*image,void*mapped,U32 seed,int cmd,void*out){
    key=seed;trace={};Remote_State=state;Remote_Entities[0]=ents;Remote_ModelMap=map;
    Remote_Models=image?static_cast<Byte*>(image)+0x3f85390:models;Remote_StringOffsets=offsets;Remote_StringBase=names;
    Remote_ClientCount=1;Remote_ClientState=cl;Remote_ErrorText=static_cast<char*>(mapped)+0x8e2000;Remote_CommandError=static_cast<char*>(mapped)+0x93d640;
    W(state,0x480e8,(key&16)?0x3ff:1);W(state,0x480ec,int((key>>5)&1));W(state,0x48088,16);
    W(ents+0x3d8,0x1f4,1);map[1]=(key&64)?1:0;
    W(state,0x6a418,int((key>>7)&1));W(state,0x64888,Byte(0x42));
    for(unsigned i=0;i<3;++i){W(state,0x4d924+4*i,float(20+i));W(tag,0x10+4*i,float(30+i));W(state,0x6a41c+4*i,float(40+i));W(state,0x649d0+4*i,float(50+i));}
    W(cl,0x44ac4,cmd);
    for(unsigned i=0;i<15;++i)W(cl,0x42cc4+(U32(cmd)&127)*0x3c+4*i,U32(cmd)+i);
    unsigned k=(key>>8)&3;Remote_Cull=k?&dvar:nullptr;W(&dvar,0x18,int(k==1?1:8));
    if(k==1)W(&dvar,0x20,Byte(key>>10));else W(&dvar,0x20,(key&1024)?"1":"0");
    if(image){
        W(image,0xe53a20,Remote_State);W(image,0xe4f8e0,Remote_Entities[0]);W(image,0x3fcd392,map[1]);
        W(image,0x2d03d98,offsets[0]);W(image,0x2d03d9c,offsets[1]);std::memcpy(static_cast<Byte*>(image)+0x2d06c3c,names,sizeof(names));
        W(image,0x1ef6b12,Remote_FallbackTag);W(image,0xe5ca9c,Remote_ClientCount);W(image,0xe61bf8,Remote_ClientState);W(image,0xd78610,Remote_Cull);
        trace.fields[0]=reinterpret_cast<bool(MS*)(int)>(static_cast<Byte*>(image)+0xc3650)(0);
    }else trace.fields[0]=GetWeaponViewEyeValues(0);
    for(unsigned i=0;i<3;++i){trace.fields[1+i]=R<U32>(state,0x4d924+4*i);trace.fields[4+i]=R<U32>(state,0x6a41c+4*i);trace.fields[7+i]=R<U32>(state,0x649d0+4*i);}
    trace.fields[10]=R<U32>(state,0x6a418);trace.fields[11]=R<Byte>(state,0x64888);std::memcpy(out,&trace,sizeof(trace));
}
}
}
