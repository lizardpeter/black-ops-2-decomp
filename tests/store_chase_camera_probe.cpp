#define T6_STORE
#define T6_STORE_CAMERA_REPLAY
#include "../src/cgame_mp/cg_chase_camera_store.cpp"
#include <cmath>
#include <cstdlib>
#include <cassert>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_chase_camera {
struct Dvar{Byte bytes[104];};
alignas(16) static Byte state[0x65000],cl[0x2cb0],play[0x1017a8];
static Dvar dvars[3];
static const char*strings[]={"0","1","2","45","90","-20","0.5","abc"};
static unsigned key;
struct Trace{U count;U events[12][6];U outputs[12];};static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static U Bits(float v){U32 b;std::memcpy(&b,&v,4);return b;}
static void Push(U k,U a=0,U b=0,U c=0,U d=0,U e=0){assert(trace.count<12);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;p[5]=e;}
extern "C" {
const Dvar *Chase_Mode,*Chase_Angle,*Chase_Range;
Byte *Chase_State,*Chase_ClientState;
int Chase_ClientCount,Chase_DemoMode;
Byte Chase_DemoFlag,Chase_PlaybackInited;
const Byte *Chase_Playback;
int Chase_Atoi(const char*s){int n=std::atoi(s);Push(1,U32(n));return n;}
double Chase_Atof(const char*s){double f=std::atof(s);Push(2,Bits(float(f)));return f;}
void Chase_DemoUpdate(int c){Push(3,c);if(key&128)W(cl,0x2ca8,123.0f);}
__m128 Chase_SinCos(float a,float b){Push(4,Bits(a),Bits(b));return _mm_setr_ps(std::sin(a),std::cos(a),std::sin(b),std::cos(b));}
float Chase_Atan2(float a,float b){Push(5,Bits(a),Bits(b));return std::atan2(a,b);}
void Chase_Trace(Byte*p,float*a,const float*b,int m,float*out,Byte x,Byte y){
    assert(p==state&&a==out&&a==reinterpret_cast<float*>(state+0x4d924));
    Push(6,Bits(b[0]),Bits(b[1]),Bits(b[2]),m,x|(y<<8));
    for(unsigned i=0;i<3;++i)out[i]=b[i]+float(i);
}
void Chase_Axis(const float*a,float*b){assert(b==reinterpret_cast<float*>(state+0x4d934));Push(7,Bits(a[0]),Bits(a[1]),Bits(a[2]));for(unsigned i=0;i<9;++i)b[i]=a[i%3]+float(i);}
}
static int MS MAtoi(const char*s){return Chase_Atoi(s);}
static double MS MAtof(const char*s){return Chase_Atof(s);}
static void MS MDemo(int c){Chase_DemoUpdate(c);}
static __m128 MS MTrig(float a,float b){return Chase_SinCos(a,b);}
static float MS MAtan(float a,float b){return Chase_Atan2(a,b);}
static void MS MTrace(Byte*p,float*a,const float*b,int m,float*out,Byte x,Byte y){Chase_Trace(p,a,b,m,out,x,y);}
static void MS MAxis(const float*a,float*b){Chase_Axis(a,b);}
extern "C" {
void*ChaseMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MDemo),reinterpret_cast<void*>(MTrig),reinterpret_cast<void*>(MAtan),reinterpret_cast<void*>(MTrace),reinterpret_cast<void*>(MAxis)};assert(i<7);return m[i];}
unsigned ChaseSize(){return sizeof(Trace);}
void ChaseRun(void*image,unsigned seed,unsigned config,U32 p,U32 y,U32 r,void*out){
    key=seed;trace={};Chase_State=state;Chase_ClientState=cl;Chase_ClientCount=1;
    Chase_DemoMode=(key&1)?2:0;Chase_DemoFlag=(key>>1)&1;Chase_PlaybackInited=(key>>2)&1;Chase_Playback=play;
    W(play,0x1017a4,int((key&8)?4:0));W(cl,0x2ca4,p);W(cl,0x2ca8,y);W(cl,0x2cac,r);W(state,0x649d4,45.0f);
    for(unsigned i=0;i<3;++i)W(state,0x4d924+4*i,float(10+i));
    const Dvar**slots[]={&Chase_Mode,&Chase_Angle,&Chase_Range};
    const float vals[]={1,45,100};
    for(unsigned i=0;i<3;++i){unsigned k=(config>>(3*i))&7;*slots[i]=k?&dvars[i]:nullptr;
        int t=k==1?2:k==2?6:k==3&&i==0?7:k==4&&i==0?1:8;
        W(&dvars[i],0x18,t);
        if(t==2)W(&dvars[i],0x20,vals[i]);else if(t==6||t==7)W(&dvars[i],0x20,int(vals[i]));else if(t==1)W(&dvars[i],0x20,Byte(1));else W(&dvars[i],0x20,strings[(key>>(8+i*3))&7]);}
    if(image){
        W(image,0xe53a20,Chase_State);W(image,0xe61bf8,Chase_ClientState);W(image,0xe5ca9c,Chase_ClientCount);
        W(image,0x25a3308,Chase_DemoMode);W(image,0x25c3368,Chase_DemoFlag);W(image,0x2653908,Chase_PlaybackInited);W(image,0x2653900,Chase_Playback);
        W(image,0xe4bae8,Chase_Mode);W(image,0xe4ba78,Chase_Angle);W(image,0xe4ba98,Chase_Range);
        reinterpret_cast<void(MS*)(int)>(static_cast<Byte*>(image)+0x16a5d0)(0);
    }else OffsetChaseCamView(0);
    for(unsigned i=0;i<3;++i)trace.outputs[i]=R<U32>(state,0x4d924+4*i);
    for(unsigned i=0;i<9;++i)trace.outputs[3+i]=R<U32>(state,0x4d934+4*i);
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
