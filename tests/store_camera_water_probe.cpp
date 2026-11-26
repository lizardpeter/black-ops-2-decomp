#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_water_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_water {
struct Dvar{Byte bytes[104];};static Dvar dvar;
static Byte table[32*32];static const char*strings[]={"0","1","-1","5","abc"};
static unsigned key;static U32 fbits,hbits;
struct Trace{U count;U calls[4][4];U ray[16],initial[16],result;};static Trace trace;
static U32 Bits(float f){U32 v;std::memcpy(&v,&f,4);return v;}
static float F(U32 v){float f;std::memcpy(&f,&v,4);return f;}
static void Push(U k,U a=0,U b=0,U c=0){assert(trace.count<4);auto*p=trace.calls[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;}
extern "C" {
const Dvar *Water_Count;
const Byte *Water_GeometryTable;
int Water_Atoi(const char*s){int n=std::atoi(s);Push(1,U32(n));return n;}
void Water_GeometryTrace(const Ray*r,const Byte*end,GeometryResult*result){
    Push(2,U(end-Water_GeometryTable));
    for(unsigned i=0;i<16;++i){trace.ray[i]=R<U32>(r,4*i);trace.initial[i]=R<U32>(result,4*i);}
    W(result,16,fbits);
}
int Water_BrushTrace(const float*a,const float*b,int mask,void*x,float*hit,void*y,void*z){
    assert(mask==32&&!x&&!y&&!z&&a[0]==b[0]&&a[1]==b[1]);
    Push(3,Bits(a[2]),Bits(b[2]),mask);hit[2]=F(hbits);return key&1;
}
}
static int MS MAtoi(const char*s){return Water_Atoi(s);}
static void MS MGeometry(const Ray*r,const Byte*e,GeometryResult*t){Water_GeometryTrace(r,e,t);}
static int MS MBrush(const float*a,const float*b,int m,void*x,float*h,void*y,void*z){return Water_BrushTrace(a,b,m,x,h,y,z);}
extern "C" {
void*WaterMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MGeometry),reinterpret_cast<void*>(MBrush)};assert(i<3);return m[i];}
unsigned WaterSize(){return sizeof(Trace);}
void WaterRun(void*image,unsigned seed,U32 z,U32 a,U32 b,U32 f,U32 h,void*out){
    key=seed;trace={};fbits=f;hbits=h;
    const unsigned kind=(key>>1)&7;Water_Count=kind?&dvar:nullptr;
    const int t=kind==1?2:kind==2?6:kind==3?7:kind==4?1:8;
    W(&dvar,0x18,t);
    if(t==2)W(&dvar,0x20,float(int((key>>4)&7)-3)*0.75f);
    else if(t==6||t==7)W(&dvar,0x20,int((key>>4)&7)-3);
    else if(t==1)W(&dvar,0x20,Byte((key>>4)&1));
    else W(&dvar,0x20,strings[(key>>4)%5]);
    Water_GeometryTable=image?static_cast<Byte*>(image)+0x210ccd0:table;
    const float origin[]={12,24,F(z)};
    float value;
    if(image){W(image,0x265ce78,Water_Count);value=reinterpret_cast<float(MS*)(const float*,float,float)>(static_cast<Byte*>(image)+0x38bad0)(origin,F(a),F(b));}
    else value=WaterHeight(origin,F(a),F(b));
    trace.result=Bits(value);std::memcpy(out,&trace,sizeof(trace));
}
}
}
