#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_transition_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_transition {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x6b900],cl[0x2cb0],def[32];static Dvar dvars[2];
static unsigned key;
struct Trace{U count;U calls[8][5];U fields[14];};static Trace trace;
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0){assert(trace.count<8);auto*p=trace.calls[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
extern "C" {
Byte *Transition_State,*Transition_ClientState;int Transition_ClientCount;
const Dvar *Transition_ExitTime,*Transition_Highlight;const Byte *Transition_VehicleDefs[256];
double Transition_Atof(const char*s){Push(1);return std::atof(s);}
int Transition_Missile(int c){Push(2,c);return key&1;}
void Transition_SetFloat(const Dvar*d,float f,int s){assert(d==Transition_Highlight);Push(3,Bits(f),s);}
void Transition_SetVariant(const Dvar*d,const Value*v,int s){assert(d==Transition_Highlight&&s==0);Push(4,v->words[0],v->words[1],v->words[2],v->words[3]);}
void Transition_StartTween(float f){Push(5,Bits(f));}
}
static double MS MAtof(const char*s){return Transition_Atof(s);}
static int MS MMissile(int c){return Transition_Missile(c);}
static void MS MFloat(const Dvar*d,float f,int s){Transition_SetFloat(d,f,s);}
static void MS MVariant(const Dvar*d,const Value*v,int s){Transition_SetVariant(d,v,s);}
// The original uses an LTCG specialized call: xmm1 contains duration,
// RCX is not prepared, and r8/r9/r10 remain live across the call.
extern "C" __attribute__((ms_abi,noinline,visibility("hidden"))) void TransitionTweenBridge(float f){Transition_StartTween(f);}
extern "C" __attribute__((ms_abi,naked)) void MTween(){
    __asm__("push %r8\n\tpush %r9\n\tpush %r10\n\tsub $32,%rsp\n\tmovaps %xmm1,%xmm0\n\tcall TransitionTweenBridge\n\tadd $32,%rsp\n\tpop %r10\n\tpop %r9\n\tpop %r8\n\tret");
}
extern "C" {
void*TransitionMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MMissile),reinterpret_cast<void*>(MFloat),reinterpret_cast<void*>(MVariant),reinterpret_cast<void*>(MTween)};assert(i<5);return m[i];}
unsigned TransitionSize(){return sizeof(Trace);}
void TransitionRun(void*image,unsigned old,unsigned next,U32 seed,void*out){
    key=seed;trace={};Transition_State=state;Transition_ClientState=cl;Transition_ClientCount=1;
    Transition_VehicleDefs[1]=def;W(def,0xc,int((key>>1)&1));W(state,0x64a34,1);
    W(state,0x64b05,Byte((key>>2)&1));W(state,0x480c0,Byte((key&8)?2:0));W(state,0x481a4,U32((key&16)?0x4000:0));
    W(state,0x48520,int((key>>5)&1));W(state,0x64a38,int((key>>6)&1));W(state,0x6b818,int((key>>7)&3));
    W(state,0x64a48,Byte((key>>9)&1));W(state,0x64b04,Byte((key>>10)&1));W(state,0x6a418,0);
    for(unsigned i=0;i<3;++i){
        W(state,0x482a0+4*i,float(10+i));W(state,0x649e8+4*i,float(20+i));W(state,0x64a3c+4*i,float(30+i));W(state,0x48140+4*i,float(40+i));W(cl,0x2ca4+4*i,float(50+i));
    }
    unsigned k=(key>>11)&3;Transition_ExitTime=k?&dvars[0]:nullptr;Transition_Highlight=&dvars[1];
    W(&dvars[0],0x18,int(k==1?2:k==2?6:8));
    if(k==1)W(&dvars[0],0x20,(key&8192)?-1.0f:0.75f);else if(k==2)W(&dvars[0],0x20,int((key&8192)?0:2));else W(&dvars[0],0x20,(key&8192)?"nan":"0.125");
    for(unsigned i=0;i<4;++i)W(&dvars[1],0x40+4*i,U32(i+123));
    if(image){
        W(image,0xe53a20,Transition_State);W(image,0xe61bf8,Transition_ClientState);W(image,0xe5ca9c,Transition_ClientCount);
        W(image,0xd78630,Transition_ExitTime);W(image,0xd78620,Transition_Highlight);W(image,0x265d0b8,Transition_VehicleDefs[1]);
        reinterpret_cast<void(MS*)(int,int,int)>(static_cast<Byte*>(image)+0xbfcc0)(0,old,next);
    }else UpdateCameraTransition(0,old,next);
    for(unsigned i=0;i<3;++i){trace.fields[i]=R<U32>(state,0x482a0+4*i);trace.fields[3+i]=R<U32>(state,0x64a3c+4*i);trace.fields[6+i]=R<U32>(cl,0x2ca4+4*i);}
    trace.fields[9]=R<Byte>(state,0x64b05);trace.fields[10]=R<Byte>(state,0x64a48);trace.fields[11]=R<U32>(state,0x6a418);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
