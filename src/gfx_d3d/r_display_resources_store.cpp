// Original Store MP display configuration and resource setup, recovered from PE bodies.
#define T6_SOURCE "gfx_d3d/r_init.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_display{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof(v));}
extern "C"{
extern int Display_Width,Display_Height;
extern std::uint16_t Display_TargetWidth,Display_TargetHeight;
extern B Display_UI3DSlots[][0xa4];
extern const B*Display_MSAADvar,*Display_AspectDvar,*Display_FullscreenDvar,*Display_BufferDvar;
extern Q Display_SampleDesc;
extern B Display_VideoConfig[0x34];
extern B Display_OverrideFullscreen;
extern U Display_OverrideWidth,Display_OverrideHeight;
extern void*Display_ClearDvar,*Display_WideDvar;
extern const char*Display_True,*Display_False;
int Display_Atoi(const char*);
void Display_Clear(void*);
void Display_SetDvar(void*,const void*,int);
void Display_AdjustRect(int*,U,int,U);
void Display_SetStyle(void*,int,int);
void Display_SetPosition(void*,void*,int,int,int,int,U);
extern U Display_ClientCount;
extern B Display_ClientQueries[][0xf08];
int Display_CreateQuery(const Q*,Q*);
extern U Display_CreatePending;
void Display_CreateBuffer(const void*,const void*,void*);
extern B Display_ResourceSlots[40][0x38];
void Display_LockResources();
void Display_UnlockResources();
extern const char*Display_TargetNames[];
extern B Display_TargetRecords[][0x68];
extern void*Display_TargetHash[2048];
extern void*Display_DefaultTexture,*Display_TextureCache[16],*Display_Context;
extern B Display_SamplerByte[16];
extern int Display_SamplerCache[16];
void Display_BindDefaultTexture(void*,U,U,void*const*);
void Display_BindSampler(void*,U,U);
}
static U DvarInt(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<U>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return U(_mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20))));
    return U(Display_Atoi(R<const char*>(d,0x20)));
}
static int Trunc(float f){return _mm_cvttss_si32(_mm_set_ss(f));}
static float FloorOriginal(float f){int i=Trunc(f);if(i!=(-2147483647-1)&&float(i)!=f)return float(i-(f<0));return f;}
// 140723A90
void SetUI3DViewport(U slot,float x,float y,float w,float h){
    if(slot>=6)return;
    x=x>0?(x<1?x:1):0;y=y>0?(y<1?y:1):0;
    if(!((1-x)-w>=0))w=1-x;
    if(!(w>0))w=0;
    if(!((1-y)-h>=0))h=1-y;
    if(!(h>0))h=0;
    B*p=Display_UI3DSlots[slot];float dw=float(Display_Width),dh=float(Display_Height);
    int wi=Trunc(dw*w),hi=Trunc(dh*h);float wf=float(wi),hf=float(hi);
    W(p,0,Trunc(dw*x));W(p,4,Trunc(dh*y));W(p,8,wi);W(p,12,hi);
    for(unsigned o=0;o<=8;o+=8){W(p,0x10+o,wf*0.0015625f);W(p,0x14+o,hf*0.0020833334f);}
    W(p,0x20,640.0f/wf);W(p,0x24,480.0f/hf);
    W(p,0x28,Q(0));W(p,0x48,Q(0));W(p,0x60,Q(0));W(p,0x80,Q(0));
    W(p,0x30,wf);W(p,0x34,hf);W(p,0x50,wf);W(p,0x54,hf);W(p,0x68,wf);W(p,0x6c,hf);
    W(p,0x88,1.0f);float sx=dw/float(Display_TargetWidth),sy=dh/float(Display_TargetHeight);
    W(p,0x8c,sx*x);W(p,0x90,sy*y);W(p,0x94,sx*w);W(p,0x98,sy*h);
}
// 14071E360
void BuildSwapChainDesc(B*out,const B*in){
    U msaa=DvarInt(Display_MSAADvar);
    Display_SampleDesc=msaa==16?Q(0x1000000004ull):Q(msaa);
    std::memset(out,0,72);
    W(out,0,R<U>(in,32));W(out,4,R<U>(in,36));W(out,16,U(0x1c));
    W(out,0x1c,U(1));W(out,0x24,U(0x20));W(out,0x28,U(2));
    W(out,0x30,R<Q>(in,0));W(out,0x38,U(1));W(out,0x40,U(2));
}
// 14071E440
void UpdateVideoConfig(const B*in){
    B*p=Display_VideoConfig;U w=R<U>(in,0x18),h=R<U>(in,0x1c),rw=R<U>(in,0x20),rh=R<U>(in,0x24);
    U full=in[0xc];W(p,0,w);W(p,4,h);W(p,8,rw);W(p,12,rh);W(p,0x14,R<U>(in,8));W(p,0x24,full);
    U a=DvarInt(Display_AspectDvar);float aspect=R<float>(p,0x28);
    if(a==0){U aw=rw,ah=rh;if(full&&Display_OverrideFullscreen){aw=Display_OverrideWidth;ah=Display_OverrideHeight;}int ratio=Trunc(FloorOriginal((float(int(ah))*16.0f)/float(int(aw))+0.5f));aspect=ratio<10?1.7777778f:ratio==10?1.6f:1.3333334f;}
    else if(a==1)aspect=1.3333334f;else if(a==2)aspect=1.6f;else if(a==3)aspect=1.7777778f;
    W(p,0x28,aspect);W(p,0x20,U(rh*rw>479999));bool wide=aspect!=1.3333334f;W(p,0x1c,U(wide));
    Display_Clear(Display_ClearDvar);
    if(Display_WideDvar&&R<Q>(Display_WideDvar,0)){Q val=0;if(R<int>(Display_WideDvar,0x18)==1)val=wide;else val=reinterpret_cast<Q>(wide?Display_True:Display_False);Display_SetDvar(Display_WideDvar,&val,0);}
    W(p,0x18,U(0));W(p,0x2c,(float(h)*aspect)/float(w));W(p,0x30,full?(float(rh)*aspect)/float(rw):1.0f);
}
// 1407206E0
void SetWindowMode(const B*in){
    void*hwnd=R<void*>(in,0);int x=R<int>(in,16),y=R<int>(in,20),w=R<int>(in,32),h=R<int>(in,36);
    U mode=DvarInt(Display_FullscreenDvar);
    if(!mode){int rect[4]={0,0,w,h};Display_AdjustRect(rect,0xc80000,0,0);Display_SetStyle(hwnd,-16,0xc80000);w=rect[2]-rect[0];h=rect[3]-rect[1];}
    else{Display_SetStyle(hwnd,-16,0);if(mode==1)x=y=0;}
    Display_SetPosition(hwnd,nullptr,x,y,w,h,100);
}
// 140724F40
void ResetClientQueries(){
    for(U c=0;c<Display_ClientCount;++c){
        B*p=Display_ClientQueries[c];W(p,0,Q(0));
        for(unsigned group=0;group<2;++group)for(unsigned i=0;i<16;++i){B*q=p+8+group*0x780+i*0x78;q[0]=0;W(q,8,Q(0));W(q,0x10,Q(0));W(q,0x18,Q(0));W(q,0x20,U(0));W(q,0x24,std::uint16_t(0));}
    }
}
// 140724FE0
void CreateQueryPairs(B*p){
    for(unsigned i=0;i<5;++i){Q desc=1,q=0;int hr=Display_CreateQuery(&desc,&q);W(p,0x28+i*8,hr<0?Q(0):q);desc=1;q=0;hr=Display_CreateQuery(&desc,&q);W(p,0x50+i*8,hr<0?Q(0):q);}
}
// 140738C00
void InitDynamicBuffer(B*p,U size){
    W(p,4,size);W(p,0,U(0));
    const B*d=Display_BufferDvar;
    if(d&&(R<int>(d,0x18)==1?d[0x20]!=0:Display_Atoi(R<const char*>(d,0x20))!=0)){
        U desc[6]={size,2,1,0x10000,0,0};do{Display_CreateBuffer(desc,nullptr,p+8);}while(Display_CreatePending!=0);
    }
    W(p,16,Q(0));
}
// 140740BE0
short AllocateResourceSlot(U a,Q /*unused*/,B kind,U count){
    Display_LockResources();short result=-1;
    for(short i=0;i<40;++i){B*p=Display_ResourceSlots[i];if(!p[0]){p[2]=kind;W(p,0x10,count);p[0]=1;p[1]=0;p[3]=0;W(p,0x14,U(0));W(p,0x18,a);W(p,0x1c,U(0));W(p,0x20,std::uint16_t(0));result=i;break;}}
    Display_UnlockResources();return result;
}
// 14073B740
B* RegisterTargetName(int id,B a,B b){
    B*p=Display_TargetRecords[id];const char*n=Display_TargetNames[id];p[9]=b;W(p,0x58,n);p[10]=a;p[15]=0;
    U hash=0;for(const char*c=n;*c;++c)hash=(hash*33)^U(static_cast<signed char>(*c));
    U slot=hash&2047,tries=0;while(Display_TargetHash[slot]){++tries;slot=(slot+1)&2047;if(tries>2048)break;}Display_TargetHash[slot]=p;return p;
}
// 1407619E0
void ResetTextureCache(){
    for(U i=0;i<16;++i){if(Display_TextureCache[i]!=Display_DefaultTexture){Display_TextureCache[i]=Display_DefaultTexture;Display_BindDefaultTexture(Display_Context,i,1,&Display_DefaultTexture);}if(Display_SamplerByte[i]!=1){Display_SamplerByte[i]=1;if(Display_SamplerCache[i]!=1){Display_BindSampler(Display_Context,i,1);Display_SamplerCache[i]=1;}}}
}
}
#endif
