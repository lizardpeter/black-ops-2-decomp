#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame/cg_camera_motion_store.cpp"
#include <cassert>
#include <cmath>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_camera_motion {
alignas(16) static Byte state[0x6b800],entity[0x3d8*2];
static unsigned key;
struct Trace{U count;U events[12][5];U output[40];};
static Trace trace;
static void Push(U k,U a=0,U b=0,U c=0,U d=0){
    assert(trace.count<12);auto *p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;
}
extern "C" {
Byte *Motion_State,*Motion_EntityBase[1];
const char *Motion_ErrorText;
__m128 Motion_SinCosPair(float a,float b){Push(1,Bits(a),Bits(b));return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
__m128 Motion_SinCosSingle(float a){Push(2,Bits(a));return _mm_setr_ps(std::sin(a),std::cos(a),0,0);}
float Motion_Cos(float a){Push(3,Bits(a));return std::cos(a);}
void Motion_MatrixMultiply(const float*a,const float*b,float*c){
    assert(reinterpret_cast<Byte*>(c)==state+0x4d934);Push(4);
    for(unsigned i=0;i<9;++i){trace.output[20+i]=Bits(a[i]);trace.output[29+i]=Bits(b[i]);c[i]=a[i]+b[i];}
}
bool Motion_ErrorCheck(){Push(5);return (key&256)!=0;}
void Motion_Error(int c,const char*s){assert(s==Motion_ErrorText);Push(6,c);}
void Motion_EntityTween(int a,int c,U32 d,int t0,int t1,float*p,float*q){
    assert(a==0&&c==0&&reinterpret_cast<Byte*>(p)==state+0x64a0c&&reinterpret_cast<Byte*>(q)==state+0x64a18);
    Push(7,d,t0,t1);
    if(key&512){for(unsigned i=0;i<3;++i){p[i]+=5;q[i]+=10;}}
}
}
static __m128 MS MPair(float a,float b){return Motion_SinCosPair(a,b);}
static __m128 MS MSingle(float a){return Motion_SinCosSingle(a);}
static float MS MCos(float a){return Motion_Cos(a);}
static void MS MMatrix(const float*a,const float*b,float*c){Motion_MatrixMultiply(a,b,c);}
static bool MS MCheck(){return Motion_ErrorCheck();}
static void MS MError(int c,const char*s){Motion_Error(c,s);}
static void MS MEntity(int a,int c,U32 d,int t0,int t1,float*p,float*q){Motion_EntityTween(a,c,d,t0,t1,p,q);}
extern "C" {
void* MotionMock(unsigned i){
    void*m[]={reinterpret_cast<void*>(MPair),reinterpret_cast<void*>(MSingle),reinterpret_cast<void*>(MCos),reinterpret_cast<void*>(MMatrix),
        reinterpret_cast<void*>(MCheck),reinterpret_cast<void*>(MError),reinterpret_cast<void*>(MEntity)};
    assert(i<7);return m[i];
}
unsigned MotionSize(){return sizeof(Trace);}
void MotionRun(void*image,void*mapped,unsigned op,unsigned seed,U32 a,U32 b,U32 c,int now,int old,U32 dur,void*out){
    key=seed;trace={};Motion_State=state;Motion_EntityBase[0]=(key&128)?nullptr:entity;
    Motion_ErrorText=static_cast<char*>(mapped)+0x8e2000;
    W(state,0x4808c,now);W(state,0x64a04,old);W(state,0x64a28,old);W(state,0x64a2c,dur);
    W(state,0x4851c,std::int16_t((key&64)?0x3ff:(key&128)?0:1));W(entity+0x3d8,0x1f4,U32(0x12345678));
    const U32 vals[]={a,b,c};
    for(unsigned i=0;i<3;++i){
        W(state,0x649d0+4*i,vals[i]);W(state,0x649e8+4*i,float(20+30*i));W(state,0x4d924+4*i,float(100+40*i));
        W(state,0x649dc+4*i,float(10+4*i));W(state,0x64a0c+4*i,float(15+i));W(state,0x64a18+4*i,float(int((key>>(3*i))&7)*100-300));
    }
    W(state,0x6b790,a);W(state,0x6b794,b);
    for(unsigned i=0;i<9;++i)W(state,0x4d934+4*i,float(i));
    for(unsigned i=0;i<4;++i)W(state,0x4dbb4+4*i,U32(0x42424242));
    if(image){
        W(image,0xe53a20,Motion_State);W(image,0xe4f8e0,Motion_EntityBase[0]);
        const auto p=static_cast<Byte*>(image);
        switch(op){
        case 0:reinterpret_cast<void(MS*)(const float*,float*)>(p+0x464d00)(reinterpret_cast<float*>(state+0x649d0),reinterpret_cast<float*>(state+0x4d934));break;
        case 1:reinterpret_cast<void(MS*)(Byte*)>(p+0x16c380)(state);break;
        case 2:reinterpret_cast<void(MS*)(Byte*)>(p+0x157bc0)(state);break;
        case 3:reinterpret_cast<void(MS*)(int)>(p+0xbf940)(0);break;
        }
    }else{
        switch(op){
        case 0:AnglesToAxis(reinterpret_cast<float*>(state+0x649d0),reinterpret_cast<float*>(state+0x4d934));break;
        case 1:UpdateCameraVelocity(state);break;
        case 2:PerturbCamera(state);break;
        case 3:UpdateCameraTween(0);break;
        }
    }
    if(op==0||op==2)for(unsigned i=0;i<9;++i)trace.output[i]=R<U32>(state,0x4d934+4*i);
    if(op==1)for(unsigned i=0;i<4;++i)trace.output[i]=R<U32>(state,0x4dbb4+4*i);
    if(op==3){
        for(unsigned i=0;i<3;++i){trace.output[i]=R<U32>(state,0x4d924+4*i);trace.output[3+i]=R<U32>(state,0x649d0+4*i);
          trace.output[6+i]=R<U32>(state,0x649dc+4*i);trace.output[9+i]=R<U32>(state,0x649e8+4*i);}
        trace.output[12]=R<U32>(state,0x64a04);
        for(unsigned i=0;i<6;++i)trace.output[13+i]=R<U32>(state,0x64a0c+4*i);
    }
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
