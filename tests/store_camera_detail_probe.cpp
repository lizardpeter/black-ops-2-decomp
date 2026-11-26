#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_detail_store.cpp"
#include <cmath>
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_detail {
alignas(16) static Byte state[0x6b800],slot[0x24],ents[0x3d8*2],models[0x90*2],tags[2][32];
static std::int16_t map[0x701];static unsigned key;
struct Trace{U count;U events[12][4];U output[6];};static Trace trace;
static U Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<12);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;}
extern "C" {
Byte *Detail_State,*Detail_Entities[1],*Detail_Models;
const std::int16_t *Detail_ModelMap;
unsigned short Detail_PrimaryTag=31,Detail_FallbackTag=32;
const char*Detail_ErrorText;
bool Detail_SuppressShake(){Push(1);return key&1;}
__m128 Detail_SinCos(float a,float b){Push(2,Bits(a),Bits(b));return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
bool Detail_ErrorCheck(){Push(3);return key&2;}
void Detail_Error(int c,const char*s){assert(s==Detail_ErrorText);Push(4,c);}
Byte*Detail_Tag(Byte*e,Byte*m,unsigned id){assert(e==ents+0x3d8&&m==Detail_Models+0x90);Push(5,id);return id==31?((key&4)?tags[0]:nullptr):((key&8)?tags[1]:nullptr);}
void Detail_TagAxis(Byte*t,float*out){assert(t==tags[1]);Push(6);for(unsigned i=0;i<9;++i)out[i]=float(i);}
void Detail_RemoteSpecial(Byte*p,Byte*e){assert(p==state&&e==ents+0x3d8);Push(7);}
bool Detail_RemoteFallback(int c){Push(8,c);return key&16;}
}
static bool MS MShake(){return Detail_SuppressShake();}
static __m128 MS MTrig(float a,float b){return Detail_SinCos(a,b);}
static bool MS MCheck(){return Detail_ErrorCheck();}
static void MS MError(int c,const char*s){Detail_Error(c,s);}
static Byte* MS MTag(Byte*e,Byte*m,unsigned id){return Detail_Tag(e,m,id);}
extern "C" __attribute__((ms_abi,visibility("hidden"),noinline)) void DetailAxisBridge(Byte*t,float*a){Detail_TagAxis(t,a);}
extern "C" __attribute__((ms_abi,naked)) void MAxis(Byte*,float*){__asm__("push %rcx\n\tsub $32,%rsp\n\tcall DetailAxisBridge\n\tadd $32,%rsp\n\tpop %rcx\n\tret");}
static void MS MSpecial(Byte*p,Byte*e){Detail_RemoteSpecial(p,e);}
static bool MS MFallback(int c){return Detail_RemoteFallback(c);}
extern "C" {
void*DetailMock(unsigned i){void*m[]={reinterpret_cast<void*>(MShake),reinterpret_cast<void*>(MTrig),reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MTag),reinterpret_cast<void*>(MAxis),reinterpret_cast<void*>(MSpecial),reinterpret_cast<void*>(MFallback)};assert(i<8);return m[i];}
unsigned DetailSize(){return sizeof(Trace);}
void DetailRun(void*image,void*mapped,unsigned op,unsigned seed,U32 a,U32 b,U32 c,int now,int start,void*out){
    trace={};key=seed;Detail_State=state;Detail_Entities[0]=ents;Detail_ModelMap=map;Detail_Models=image?static_cast<Byte*>(image)+0x3f85390:models;Detail_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    W(state,0x4808c,now);W(slot,0,start);W(slot,4,a);W(slot,8,b);W(slot,0xc,c);
    for(unsigned i=0;i<3;++i){W(state,0x4d924+4*i,float(i*10));W(slot,0x10+4*i,float(i*2));W(tags[0],0x10+4*i,float(30+i));W(tags[1],0x10+4*i,float(60+i));}
    W(slot,0x1c,U32(0x42424242));W(slot,0x20,U32(0x42424242));
    W(state,0x4852c,Byte((key&32)?4:0));W(state,0x480c0,U32((key&64)?0x2000:0));W(state,0x4851c,std::int16_t(1));
    Byte*e=ents+0x3d8;W(e,0x3cc,U32((key&128)?2:0));W(e,0x1f4,1);W(e,0x2cc,static_cast<unsigned short>((key&256)?12:0));map[1]=(key&512)?1:0;
    if(image){
        W(image,0xe53a20,Detail_State);W(image,0xe4f8e0,Detail_Entities[0]);W(image,0x3fcd392,map[1]);W(image,0x1ef6b18,Detail_PrimaryTag);W(image,0x1ef6b12,Detail_FallbackTag);
        auto p=static_cast<Byte*>(image);
        switch(op){case 0:trace.output[0]=reinterpret_cast<int(MS*)(Byte*,Byte*)>(p+0xc72b0)(state,slot);break;
        case 1:reinterpret_cast<void(MS*)(float*,float,float)>(p+0x472fe0)(reinterpret_cast<float*>(state+0x4d924),F(a),F(b));break;
        case 2:trace.output[0]=reinterpret_cast<bool(MS*)(int)>(p+0xc3950)(0);break;}
    }else switch(op){case 0:trace.output[0]=UpdateShake(state,slot);break;
        case 1:OffsetOrigin(reinterpret_cast<float*>(state+0x4d924),F(a),F(b));break;
        case 2:trace.output[0]=CalcRemoteEyesView(0);break;}
    trace.output[1]=R<U32>(slot,0x1c);trace.output[2]=R<U32>(slot,0x20);
    for(unsigned i=0;i<3;++i)trace.output[3+i]=R<U32>(state,0x4d924+4*i);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
