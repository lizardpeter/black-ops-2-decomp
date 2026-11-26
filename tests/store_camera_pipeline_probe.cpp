#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_fov_defaults_store.cpp"
#include "../src/cgame/cg_camera_projection_store.cpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdint>
using U=std::uint64_t;
using U32=std::uint32_t;
using Byte=unsigned char;
#define MS __attribute__((ms_abi))
static Byte cg[0x70000],statics[32],dvars[5][104];
static const char *strings[]={"0","1","65","90","nan","inf","-3","40"};
static const char *zero="0";
struct Event{U kind,a,b,c;};
struct Trace{U count;Event events[16];U outputs[10];};
static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static U Id(const void *d){if(!d)return 0;for(unsigned i=0;i<5;++i)if(d==dvars[i])return i+1;std::abort();}
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<16);trace.events[trace.count++]={k,a,b,c};}
static double Atof(const char*s){unsigned i=0;while(i<8&&s!=strings[i])++i;assert(i<8);Push(1,i);return std::atof(s);}
static int Atoi(const char*s){unsigned i=0;while(i<8&&s!=strings[i])++i;assert(i<8);Push(2,i);return std::atoi(s);}
namespace store_fov_defaults {
extern "C" {
Byte *FovDefaults_State;
const Dvar *FovDefaults_Current,*FovDefaults_First,*FovDefaults_Third;
double FovDefaults_Atof(const char*s){return Atof(s);}
void FovDefaults_SetFloat(const Dvar*d,float f,int src){Push(3,Id(d),Bits(f),src);}
}
}
namespace store_camera_projection {
extern "C" {
Byte *Projection_State;
const Byte *Projection_Static;
const Dvar *Projection_DualEnable,*Projection_DualActive,*Projection_Fov;
int Projection_ForceFullscreen;
const char *Projection_Zero;
float Projection_GetViewFov(int client,Byte weapon){Push(4,client,weapon);return weapon?72.0f:85.0f;}
float Projection_Tan(float a){Push(5,Bits(a));return std::tan(a);}
int Projection_Atoi(const char*s){return Atoi(s);}
double Projection_Atof(const char*s){return Atof(s);}
void Projection_SetVariant(const Dvar*d,const Value*v,int src){
    if(R<int>(d,0x18)==1){assert((v->words[0]&255)==0);Push(6,Id(d),0,src);}
    else{assert(v->string && !std::strcmp(v->string,"0"));Push(6,Id(d),1,src);}
}
}
}
static double MS MAtof(const char*s){return Atof(s);}
static int MS MAtoi(const char*s){return Atoi(s);}
static void MS MSetFloat(const store_fov_defaults::Dvar*d,float f,int src){store_fov_defaults::FovDefaults_SetFloat(d,f,src);}
static float MS MGetFov(int c,Byte w){return store_camera_projection::Projection_GetViewFov(c,w);}
static float MS MTan(float a){return store_camera_projection::Projection_Tan(a);}
static void MS MVariant(const store_camera_projection::Dvar*d,const store_camera_projection::Value*v,int src){store_camera_projection::Projection_SetVariant(d,v,src);}
static const void *Make(unsigned slot,unsigned kind,float v,unsigned stringCase,bool named=true) {
    if(!kind)return nullptr;
    W(dvars[slot],0,named?strings[0]:static_cast<const char*>(nullptr));
    W(dvars[slot],0x18,int(kind==1?2:kind==2?6:kind==3?1:8));
    if(kind==1)W(dvars[slot],0x20,v);
    else if(kind==2)W(dvars[slot],0x20,int(v));
    else if(kind==3)W(dvars[slot],0x20,Byte(v!=0));
    else W(dvars[slot],0x20,strings[stringCase%8]);
    return dvars[slot];
}
extern "C" {
void *CameraPipelineMock(unsigned i){
    void *m[]={reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MSetFloat),
        reinterpret_cast<void*>(MGetFov),reinterpret_cast<void*>(MTan),reinterpret_cast<void*>(MVariant)};
    assert(i<6);return m[i];
}
unsigned CameraPipelineSize(){return sizeof(Trace);}
void DefaultFovRun(void*image,unsigned op,unsigned flags,unsigned config,unsigned values,void*out) {
    using namespace store_fov_defaults;
    trace={};FovDefaults_State=cg;cg[0x4809c]=Byte(flags);cg[0x4812c]=Byte(flags>>8);
    const float floats[]={0,65,90,40,100,-1,65,90};
    FovDefaults_Current=static_cast<const Dvar*>(Make(0,config&3,floats[values&7],(values>>9)&7));
    FovDefaults_First=static_cast<const Dvar*>(Make(1,(config>>2)&3,floats[(values>>3)&7],(values>>12)&7));
    FovDefaults_Third=static_cast<const Dvar*>(Make(2,(config>>4)&3,floats[(values>>6)&7],(values>>15)&7));
    // Type 3 is a string here; the float getter treats non-float/int as string.
    for(unsigned i=0;i<3;++i)if(((config>>(2*i))&3)==3){W(dvars[i],0x18,8);W(dvars[i],0x20,strings[(values>>(9+3*i))&7]);}
    if(image){
        W(image,0xe53a20,FovDefaults_State);W(image,0xe4f8e8,FovDefaults_Current);
        W(image,0x2161688,FovDefaults_First);W(image,0x2161678,FovDefaults_Third);
        const unsigned rvas[]={0xf9280,0xf92f0,0xf93e0,0xf9470};
        Byte *p=static_cast<Byte*>(image)+rvas[op];
        if(op==0)trace.outputs[0]=Bits(reinterpret_cast<float(MS*)()>(p)());
        else if(op==2)reinterpret_cast<void(MS*)(int,Byte)>(p)(0,Byte(flags>>16));
        else reinterpret_cast<void(MS*)()>(p)();
    }else{
        switch(op){
        case 0:trace.outputs[0]=Bits(GetDefaultFovForView());break;
        case 1:ClearViewModeAndSync();break;
        case 2:SetViewModeAndSync(0,Byte(flags>>16));break;
        case 3:SyncViewMode();break;
        }
    }
    trace.outputs[1]=cg[0x4809c];trace.outputs[2]=cg[0x4812c];std::memcpy(out,&trace,sizeof(trace));
}
void ProjectionRun(void*image,void*mapped,U32 requested,U32 aspect,unsigned config,int now,int start,U32 duration,U32 initial,void*out) {
    using namespace store_camera_projection;
    trace={};Projection_State=cg;Projection_Static=statics;
    W(statics,0x10,F(aspect));
    W(cg,0x4808c,now);W(cg,0x64a28,start);W(cg,0x64a2c,F(duration));W(cg,0x64a24,F(initial));
    Projection_DualEnable=static_cast<const Dvar*>(Make(0,config&7,1,(config>>12)&7));
    Projection_DualActive=static_cast<const Dvar*>(Make(1,(config>>3)&7,1,0,(config&512)!=0));
    Projection_Fov=static_cast<const Dvar*>(Make(2,(config>>6)&7,65,(config>>15)&7));
    if(((config>>6)&7)==3){W(dvars[2],0x18,8);W(dvars[2],0x20,strings[(config>>15)&7]);}
    // For dual enable only bool or string are meaningful for the inlined read.
    if(Projection_DualEnable && R<int>(Projection_DualEnable,0x18)!=1){
        W(dvars[0],0x18,8);W(dvars[0],0x20,strings[(config>>12)&7]);
    }
    Projection_ForceFullscreen=(config&1024)!=0;
    Projection_Zero=image?static_cast<char*>(mapped)+0x93ed9c:zero;
    if(image){
        W(image,0xe53a20,Projection_State);W(image,0xe53b70,Projection_Static);
        W(image,0x27e6e50,Projection_DualEnable);W(image,0x27e6d70,Projection_DualActive);
        W(image,0xe4f8e8,Projection_Fov);W(image,0x2cc0a2c,Projection_ForceFullscreen);
        reinterpret_cast<void(MS*)(int,float)>(static_cast<Byte*>(image)+0xc2060)(0,F(requested));
    }else CalcFov(0,F(requested));
    const unsigned offsets[]={0x649f4,0x64950,0x64954,0x4d910,0x4d914,0x4d918,0x4d920,0x69fa8,0x64a28};
    for(unsigned i=0;i<9;++i)trace.outputs[i]=R<U32>(cg,offsets[i]);
    std::memcpy(out,&trace,sizeof(trace));
}
}
