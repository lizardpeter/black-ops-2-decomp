#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_angles_store.cpp"
#include <cmath>
#include <cstdlib>
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_angles {
struct Dvar{Byte bytes[104];};static Dvar dvar;
static float input[9],out[3],link[3];
struct Trace{U count;U events[8][10];U result[3];};static Trace trace;
static U Bits(float v){U32 b;std::memcpy(&b,&v,4);return b;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static U*Event(U k){assert(trace.count<8);auto*p=trace.events[trace.count++];p[0]=k;return p;}
extern "C" {
const Dvar *Angle_RollLimit;
void Angle_VectorToAngles(const float*a,float*b){assert(a==input&&b==out);Event(1);b[0]=link[0];b[1]=link[1];}
__m128 Angle_SinCos(float a,float b){auto*p=Event(2);p[1]=Bits(a);p[2]=Bits(b);return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
float Angle_Atan2(float a,float b){auto*p=Event(3);p[1]=Bits(a);p[2]=Bits(b);return std::atan2(a,b);}
void Angle_ToAxis(const float*a,float*b){auto*p=Event(4);for(unsigned i=0;i<3;++i)p[i+1]=Bits(a[i]);for(unsigned i=0;i<9;++i)b[i]=(a[i%3]+float(i))*0.125f;}
void Angle_FromAxis(const float*a,float*b){auto*p=Event(5);for(unsigned i=0;i<9;++i)p[i+1]=Bits(a[i]);b[0]=a[0];b[1]=a[4];b[2]=a[8];}
double Angle_Atof(const char*s){Event(6);return std::atof(s);}
}
static void MS MV(const float*a,float*b){Angle_VectorToAngles(a,b);}
static __m128 MS MT(float a,float b){return Angle_SinCos(a,b);}
static float MS MA(float a,float b){return Angle_Atan2(a,b);}
static void MS MTo(const float*a,float*b){Angle_ToAxis(a,b);}
static void MS MFrom(const float*a,float*b){Angle_FromAxis(a,b);}
static double MS MF(const char*s){return Angle_Atof(s);}
extern "C" {
void*AngleMock(unsigned i){void*m[]={reinterpret_cast<void*>(MV),reinterpret_cast<void*>(MT),reinterpret_cast<void*>(MA),reinterpret_cast<void*>(MTo),reinterpret_cast<void*>(MFrom),reinterpret_cast<void*>(MF)};assert(i<6);return m[i];}
unsigned AngleSize(){return sizeof(Trace);}
void AngleRun(void*image,unsigned op,unsigned seed,U32 a,U32 b,U32 c,void*result){
    const unsigned key=seed;trace={};const float vals[]={F(a),F(b),F(c)};
    for(unsigned i=0;i<3;++i){link[i]=vals[i];out[i]=vals[2-i];}
    for(unsigned i=0;i<9;++i)input[i]=((key>>i)&1)?vals[i%3]:0.0f;
    unsigned kind=(key>>9)&3;Angle_RollLimit=kind?&dvar:nullptr;
    W(&dvar,0x18,int(kind==1?2:kind==2?6:8));
    if(kind==1)W(&dvar,0x20,(key&4096)?90.0f:85.0f);
    else if(kind==2)W(&dvar,0x20,(key&4096)?90:85);
    else W(&dvar,0x20,(key&4096)?"90":"85");
    if(image){
        W(image,0x265cf30,Angle_RollLimit);
        reinterpret_cast<void(MS*)(const float*,float*)>(static_cast<Byte*>(image)+(op?0x621c10:0x462d70))(op?link:input,out);
    }else if(op)TransformLinkedRoll(link,out);else AxisToAngles(input,out);
    for(unsigned i=0;i<3;++i)trace.result[i]=Bits(out[i]);
    std::memcpy(result,&trace,sizeof(trace));
}
}
}
