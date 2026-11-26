#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/bgame/bg_camera_bob_store.cpp"
#include <cmath>
#include <cstdlib>
#include <cassert>
using U=std::uint64_t;
using U32=std::uint32_t;
#define MS __attribute__((ms_abi))
namespace store_camera_bob {
struct Dvar{Byte bytes[104];};
static Byte ps[0x5bc];
static Dvar dvars[9];
static const char *strings[]={"0","1","-1","3.75","8"};
struct Trace{U count;U events[4][2];U result;};static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static U Bits(float v){U32 n;std::memcpy(&n,&v,4);return n;}
static float F(U32 n){float v;std::memcpy(&v,&n,4);return v;}
static void Push(U k,U a){assert(trace.count<4);trace.events[trace.count][0]=k;trace.events[trace.count++][1]=a;}
extern "C" {
const Dvar *Bob_HighState,*Bob_Flag21,*Bob_State11,*Bob_State40,*Bob_State40Ads,*Bob_Flag15,*Bob_Default,*Bob_DefaultAds,*Bob_Max;
double Bob_Atof(const char*s){unsigned i=0;while(i<5&&strings[i]!=s)++i;assert(i<5);Push(1,i);return std::atof(s);}
float Bob_Sin(float f){Push(2,Bits(f));return std::sin(f);}
}
static double MS MAtof(const char*s){return Bob_Atof(s);}
static float MS MSin(float f){return Bob_Sin(f);}
extern "C" {
void*BobMock(unsigned i){return i?reinterpret_cast<void*>(MSin):reinterpret_cast<void*>(MAtof);}
unsigned BobSize(){return sizeof(Trace);}
void BobRun(void*image,unsigned op,U32 key,U32 phase,U32 speed,U32 fraction,void*out){
    trace={};W(ps,0x5b8,int((key>>9)&7));W(ps,0xc,unsigned(((key&4096)?0x200000:0)|((key&8192)?0x8000:0)));
    const int states[]={0,11,40,7};W(ps,0x204,states[(key>>14)&3]);W(ps,0x1e0,fraction);
    const Dvar**slots[]={&Bob_HighState,&Bob_Flag21,&Bob_State11,&Bob_State40,&Bob_State40Ads,&Bob_Flag15,&Bob_Default,&Bob_DefaultAds,&Bob_Max};
    const unsigned rvas[]={0x265cce8,0x265cb48,0x265caf0,0x265cb08,0x265cc40,0x265a8a0,0x265cce0,0x265c9f0,0x265cb30};
    for(unsigned i=0;i<9;++i){
        *slots[i]=(key&(1u<<i))?&dvars[i]:nullptr;
        W(&dvars[i],0x20,float(i+1)*0.25f);W(&dvars[i],0x24,float(i+1)*0.375f);
        if(image)W(image,rvas[i],*slots[i]);
    }
    const unsigned kind=(key>>16)&3;
    W(&dvars[8],0x18,int(kind==0?2:kind==1?6:8));
    if(kind==0)W(&dvars[8],0x20,2.5f);
    else if(kind==1)W(&dvars[8],0x20,3);
    else W(&dvars[8],0x20,strings[(key>>18)%5]);
    const float result=image?reinterpret_cast<float(MS*)(const Byte*,float,float)>(static_cast<Byte*>(image)+(op?0x660470:0x6602a0))(ps,F(phase),F(speed)):
        op?HorizontalBob(ps,F(phase),F(speed)):VerticalBob(ps,F(phase),F(speed));
    trace.result=Bits(result);std::memcpy(out,&trace,sizeof(trace));
}
}
}
