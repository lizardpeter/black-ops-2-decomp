#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_basis_store.cpp"
#include <cassert>
#include <cmath>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_basis {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x65000],entity[0x2d0],tag[32];
static Dvar dvars[6];
struct Trace{U count;U calls[4][3];U result[24];};static Trace trace;
static U Bits(float f){U32 b;std::memcpy(&b,&f,4);return b;}
static void Push(U k,float a,float b=0){assert(trace.count<4);auto*p=trace.calls[trace.count++];p[0]=k;p[1]=Bits(a);p[2]=Bits(b);}
extern "C" {
const Dvar *Basis_RemoteAmplitudeA,*Basis_RemoteRateA,*Basis_RemoteLimitsA,*Basis_RemoteAmplitudeB,*Basis_RemoteRateB,*Basis_RemoteLimitsB;
float Basis_Atan2(float a,float b){Push(1,a,b);return std::atan2(a,b);}
float Basis_Sin(float a){Push(2,a);return std::sin(a);}
}
static float MS MAtan(float a,float b){return Basis_Atan2(a,b);}
static float MS MSin(float a){return Basis_Sin(a);}
extern "C" {
void*BasisMock(unsigned i){return i?reinterpret_cast<void*>(MSin):reinterpret_cast<void*>(MAtan);}
unsigned BasisSize(){return sizeof(Trace);}
void BasisRun(void*image,unsigned op,U32 key,U32 a,U32 b,U32 c,U32 d,void*out){
    trace={};float in[3]={F(a),F(b),F(c)},result[9]={};
    W(tag,0,a);W(tag,4,b);W(tag,8,c);W(tag,12,d);W(tag,0x1c,(key&1)?2.0f:0.375f);
    W(state,0xc,I(key%9u)-1);W(state,0x10,I(d));
    W(state,0x480d0,a);W(state,0x480d4,b);W(state,0x480d8,c);W(state,0x482b0,5.25f);
    const unsigned fields[]={0x4d8f0,0x4d8f4,0x4d8f8,0x4d8fc,0x4d910,0x4d914,0x4d924,0x4d928,0x4d92c,0x4d934,0x4d938,0x4d93c,0x4d940,0x4d944,0x4d948,0x4d94c,0x4d950,0x4d954};
    for(unsigned o:fields)W(state,o,U32(0x42424242));
    W(state,0x4d95c,I(key));W(entity,0x2cc,static_cast<unsigned short>((key&1)?12:1));
    W(entity,0x218,a);W(entity,0x21c,b);W(entity,0x220,c);
    for(unsigned i=0;i<3;++i)W(state,0x649d0+4*i,float(10+i));
    const Dvar**slots[]={&Basis_RemoteAmplitudeA,&Basis_RemoteRateA,&Basis_RemoteLimitsA,&Basis_RemoteAmplitudeB,&Basis_RemoteRateB,&Basis_RemoteLimitsB};
    const unsigned rvas[]={0xe5a680,0xe5a688,0xe5a678,0xe5a660,0xe5a668,0xe5a658};
    for(unsigned i=0;i<6;++i){
        *slots[i]=(key&(2u<<i))?&dvars[i]:nullptr;
        for(unsigned j=0;j<3;++j)W(&dvars[i],0x20+4*j,float(i*3+j+1)*0.125f);
        if(i%3==2){W(&dvars[i],0x20,F(d));W(&dvars[i],0x24,(key&256)?1.0f:100.0f);}
        if(image)W(image,rvas[i],*slots[i]);
    }
    if(image){auto*p=static_cast<Byte*>(image);switch(op){
        case 0:reinterpret_cast<void(MS*)(const float*,float*)>(p+0x461020)(in,result);break;
        case 1:reinterpret_cast<void(MS*)(const Byte*,float*)>(p+0xde970)(tag,result);break;
        case 2:reinterpret_cast<void(MS*)(Byte*)>(p+0x16bda0)(state);break;
        case 3:reinterpret_cast<void(MS*)(Byte*,const Byte*)>(p+0xc33f0)(state,entity);break;
    }}else switch(op){case 0:VectorToAngles(in,result);break;case 1:TagAxis(tag,result);break;
        case 2:CalcCubemapViewValues(state);break;case 3:RemoteViewMotion(state,entity);break;}
    if(op<2)for(unsigned i=0;i<(op?9u:3u);++i)trace.result[i]=Bits(result[i]);
    if(op==2){unsigned i=0;for(unsigned o:fields)trace.result[i++]=R<U32>(state,o);}
    if(op==3)for(unsigned i=0;i<3;++i)trace.result[i]=R<U32>(state,0x649d0+i*4);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
