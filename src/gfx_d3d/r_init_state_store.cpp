// Store MP renderer source/state initialization and frame issue.
#define T6_SOURCE "gfx_d3d/r_state.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_renderer_init_state{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
static void Zero(B*p,unsigned n){for(unsigned i=0;i<n;++i)p[i]=0;}
extern "C"{
extern const B*State_DefaultSource,*State_Mode;
extern const U*State_ConstantTypes;
extern const float*State_ModeColors;
int State_Atoi(const char*);
void State_SetScissors(void*,unsigned,const int*);
void State_SetBlend(void*,void*,const float*,U);
void State_SetDepth(void*,void*,unsigned);
void State_SetRaster(void*,void*);
void State_SetSampler(void*,U,U);
extern B*Frame_Data;
extern const B*Frame_Enable,*Frame_Dual,*Frame_Notify;
extern unsigned Frame_Count,Frame_Main,Frame_Render,Frame_TessCount;
extern Q Frame_LastTess;
extern float Frame_Fractions[];
extern B Frame_Threaded,Frame_Pending,Frame_GpuReady;
extern void*Frame_ProfileCounters[25];
unsigned Frame_ThreadId();
void Frame_Fallback();
void Frame_MainSync();
void Frame_ClearCounter(void*,int);
void Frame_Begin();
void Frame_Submit(B*);
void Frame_Finish();
void Frame_Lock();
void Frame_Unlock();
void Frame_Execute();
void Frame_SignalFirst();
void Frame_SignalSecond();
void Frame_Present();
void Frame_Reconfigure();
void Frame_End();
extern B Init_MeshEnabled;
extern B*Init_DrawWorld;
extern U Init_DrawCount;
extern B Init_DrawReady,Init_DrawDirty,Init_DrawEnabled;
void Init_DrawPrepare();
void Init_DrawBegin();
void Init_DrawSort(B*,B*,U,void*);
extern void*Init_DrawSortCallback;
extern const B* Init_ImageEnabled;
void Init_Image2D(B*,H,H,U,U,U,void*);
void Init_ImageCube(B*,H,U,U,void*);
void Init_Image3D(B*,H,H,U,U,U,void*);
void Init_ImageVolume(B*,H,H,H,U,U,U,void*);
void Init_ImageFinalize(B*,U,U,U,U,U);
}
static bool Bool(const B*d){return d&&(R<int>(d,0x18)==1?d[0x20]!=0:State_Atoi(R<const char*>(d,0x20))!=0);}
static U Int(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<U>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return U(_mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20))));
    return U(State_Atoi(R<const char*>(d,0x20)));
}
static void Revision(B*s,unsigned off){W(s,off,H(R<H>(s,off)+1));}
// 1407645E0
void InitializeSourceState(B*s,const B*source,U viewMode){
    Zero(s,0x1b70);std::memcpy(s+0x800,source,0xf30);
    for(unsigned i=0;i<211;++i){
        if(source==State_DefaultSource||State_ConstantTypes[i]-2u>1){
            W(s,0x800+i*16,0x7f7fffffu);W(s,0x804+i*16,0x7f7fffffu);W(s,0x808+i*16,Q(0x7f7fffff));
        }else W(s,0x18c0+i*2,H(1));
    }
    W(s,0x1af8,1.0f);W(s,0x1b08,1.0f);W(s,0x1b0c,viewMode);
    std::memcpy(s+0xac0,State_ModeColors+static_cast<long long>(int(Int(State_Mode)))*4,16);Revision(s,0x1918);
    Zero(s+0xad0,16);Revision(s,0x191a);
    for(unsigned i=0;i<6;++i){Zero(s+0x14d0+i*16,16);Revision(s,0x1a5a+i*2);}
}
// 140764AC0
void InitializeCommandState(B*s,void*data){
    void*ctx=R<void*>(s,0xd0);
    Zero(s,0xd0);Zero(s+0xd8,0x10b8);Zero(s+0x11b0,0x34);Zero(s+0x11f0,0x20c0);Zero(s+0x32d0,0x20c0);
    Zero(s+0x53b0,0x54);Zero(s+0x5405,11);Zero(s+0x5418,8);W(s,0xd0,ctx);
    const unsigned dst[12]={0x32b0,0x5390,0x1190,0x32b8,0x5398,0x1198,0x32c0,0x53a0,0x11a0,0x32c8,0x53a8,0x11a8};
    const unsigned src[12]={0x11f0,0x32d0,0x130,0x2510,0x45f0,0xac0,0x2d30,0x4e10,0xed0,0x3130,0x5210,0x10d0};
    for(unsigned i=0;i<12;++i)W(s,dst[i],s+src[i]);
    W(s,0x5410,data);W(s,0x11e4,0xffffffffu);W(s,0x11e8,Q(0x3f800000));s[0x5404]=0;
    const int rect[4]={0,0,8192,8192};State_SetScissors(ctx,1,rect);
    State_SetBlend(ctx,nullptr,nullptr,0xffffffffu);State_SetDepth(ctx,nullptr,0);State_SetRaster(ctx,nullptr);
    for(unsigned i=0;i<16;++i){State_SetSampler(R<void*>(s,0xd0),i,1);s[i]=1;W(s,0x10+i*4,1u);}
    W(s,0x120,Q(0xffffffff));W(s,0x53d0,Q(0xffffffff));W(s,0x5408,0u);
}
// 14071BAC0. The source records renderer-side counters and issues a frame;
// platform and D3D work is kept behind the original dependency boundaries.
void IssueRenderFrame(U flags){
    W(Frame_Data,0x4c0054,flags);W(Frame_Data,0x4c0050,0u);
    if(Bool(Frame_Enable))W(Frame_Data,0x4c0050,Bool(Frame_Dual)?1u:2u);
    if(flags&2)++Frame_Count;
    if(!Frame_Threaded){Frame_Fallback();return;}
    if(Frame_ThreadId()==Frame_Main)Frame_MainSync();
    if(Frame_ThreadId()==Frame_Main)for(void*p:Frame_ProfileCounters)Frame_ClearCounter(p,0);
    Frame_LastTess=0;
    Frame_Fractions[R<U>(Frame_Data,0x4bea50)]=float(R<int>(Frame_Data,0x4be9c8))/float(R<int>(R<const B*>(Frame_Data,0x4be9c0),4));
    Frame_Begin();Frame_Submit(Frame_Data);Frame_Finish();
    Frame_Lock();Frame_Execute();Frame_Unlock();
    if(Frame_ThreadId()==Frame_Render){Frame_SignalFirst();Frame_SignalSecond();}
    Frame_Lock();
    if(Frame_Data[0x4c0054]&2){
        Frame_Present();const B*d=Frame_Notify;
        if(Frame_Pending||(d&&d[0x1c])){
            Frame_Pending=0;const_cast<B*>(Frame_Notify)[0x1c]=0;
            if(!Bool(d)&&Frame_GpuReady)Frame_Reconfigure();
        }
    }
    Frame_Unlock();Frame_End();
}
// 14071D9F0
void PrepareWorldDraw(){
    if(!Init_DrawEnabled)return;
    if(Frame_ThreadId()==Frame_Main)Init_DrawPrepare();
    if(Init_DrawWorld&&Init_DrawReady&&Init_DrawDirty){
        Init_DrawReady=0;
        if(Frame_ThreadId()==Frame_Main)Frame_MainSync();
        Frame_Begin();Init_DrawBegin();
        B*w=Init_DrawWorld;const B*surfaces=R<const B*>(w,0x4d8);
        U n=R<U>(R<const B*>(w,0x320),0x38);
        if(n){Zero(R<B*>(w,0x4f0),n);Zero(R<B*>(w,0x4f8),n);}
        for(U i=0;i<n;++i){
            const B*entry=surfaces+i*0x60;const B*material=R<const B*>(entry,0x30);
            if(material[8]&0x40){R<B*>(w,0x4f8)[i]=1;if(entry[0x3b]&1)R<B*>(w,0x4f0)[i]=1;}
            W(R<B*>(w,0x4e8),i*8,(Q(entry[0x3a])<<44)|(R<Q>(material,16)&0xfff00ffffe00ffffull));
        }
        for(U i=0;i<R<U>(Init_DrawWorld,300);++i)W(R<B*>(Init_DrawWorld,0x448),i*0x18,H(0));
        for(U i=0;i<R<U>(R<const B*>(Init_DrawWorld,0x320),0x38);++i)
            Init_DrawSort(Init_DrawWorld,R<B*>(Init_DrawWorld,0x4d8)+i*0x60,i,Init_DrawSortCallback);
    }
    Init_DrawCount=1;Frame_LastTess=R<Q>(R<const B*>(Frame_Data,0x4be9c0),16);
}
// 1407776A0: dimensions are stored even when the renderer is disabled.
void SetupImage(B*image,U width,U height,U depth,U flags,U format,void*data){
    W(image,0x18,H(width));W(image,0x1a,H(height));W(image,0x1c,H(depth));
    if(Bool(Init_ImageEnabled)){
        U mip=(flags>>1)&1;
        if(flags&4)Init_ImageCube(image,R<H>(image,0x18),mip,format,data);
        else if(flags&8)Init_ImageVolume(image,R<H>(image,0x18),R<H>(image,0x1a),R<H>(image,0x1c),mip,flags,format,data);
        else Init_Image2D(image,R<H>(image,0x18),R<H>(image,0x1a),mip,flags,format,data);
        Init_ImageFinalize(image,flags,format,width,height,depth);
    }
}
}
#endif
