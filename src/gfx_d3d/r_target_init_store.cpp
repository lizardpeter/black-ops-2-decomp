// Store MP 140752AA0: original render-target initialization.
// Names describe the observed operations; external leaf/COM boundaries remain explicit.
// Fixed-height postprocessing targets are intentionally NOT changed to display resolution.
#define T6_SOURCE "gfx_d3d/r_rendertarget.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_target_init{
using U=std::uint32_t;using H=std::uint16_t;using B=unsigned char;using Q=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
template<class F>static F Method(void*o,unsigned slot){return reinterpret_cast<F>(R<void*>(R<void*>(o,0),slot));}
static void Release(void*o){Method<U(*)(void*)>(o,0x10)(o);}
static void Retain(void*o){if(o)Method<U(*)(void*)>(o,8)(o);}
extern "C"{
extern B* Targets_Records; // 142C3F7B0, records spaced by 32 bytes
extern void* Targets_Device,*Targets_SwapChain,*Targets_StoredSwapChain,*Targets_BackBuffer,*Targets_StoredRTV;
extern const B* Targets_BackBufferIID;
extern U Targets_SceneWidth,Targets_SceneHeight,Targets_DisplayWidth,Targets_DisplayHeight;
extern volatile U Targets_Retry;
extern B Targets_NoDepth;
extern const B* Targets_ShadowDvar,*Targets_SpotDvar;
extern H Targets_ShadowSize,Targets_SpotSize;
extern void* Targets_VendorObject;
extern B Targets_AuxEnabled,Targets_DeferredEnabled,Targets_AuxByte;
extern U Targets_AuxWidth,Targets_AuxHeight,Targets_DeferredArgs[5];
int Targets_ParseInt(const char*);
// 140752780: RDX is not assigned at these call sites; adapter exposes the used lanes.
void Targets_CreateRelative(U id,U shift,U format,B*record,bool flag);
void Targets_CreateSized(U id,H width,H height,U format,U flag,B*record); // 140752470
void Targets_CreateSlice(U id,B*record,void*resource,U format,U slice); // 140752360
void Targets_Attach(U id,B*record); // 1407529A0
B* Targets_Register(U id,B type,B flag); // 14073B740
void Targets_InitializeImage(void*,U,U,H,H,U); // 140776EA0
void Targets_VendorMode(void*,U); // 14007ED30
void Targets_AllocateAux(U,U,B); // 140753A00
void Targets_AllocateDeferred(U,U,U,U,U); // 140753700
}
static int IntValue(const B*d){
    if(!d)return 0;
    U t=R<U>(d,0x18);
    if(t==6||t==7)return R<int>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return _mm_cvtt_ss2si(_mm_load_ss(reinterpret_cast<const float*>(d+0x20)));
    return Targets_ParseInt(R<const char*>(d,0x20));
}
static B* Rec(unsigned offset){return Targets_Records+offset;}
static void Relative(U id,U shift,U fmt,unsigned off,bool flag=false){Targets_CreateRelative(id,shift,fmt,Rec(off),flag);}
static void Sized(U id,H w,H h,U fmt,U flag,unsigned off){Targets_CreateSized(id,w,h,fmt,flag,Rec(off));}
static void Duplicate(unsigned dst,unsigned src,bool clearView){
    std::memcpy(Rec(dst),Rec(src),32);
    if(clearView){W(Rec(dst),8,Q(0));Retain(R<void*>(Rec(dst),16));}
    else{Retain(R<void*>(Rec(dst),8));Retain(R<void*>(Rec(dst),16));}
}
void InitializeRenderTargets(){
    W(Rec(0x20),24,H(Targets_SceneWidth));W(Rec(0x20),26,H(Targets_SceneHeight));
    do{Method<int(*)(void*,U,const void*,void**)>(Targets_SwapChain,0x48)(Targets_SwapChain,0,Targets_BackBufferIID,&Targets_BackBuffer);}while(Targets_Retry);
    Targets_StoredSwapChain=Targets_SwapChain;
    do{Method<int(*)(void*,void*,const void*,void*)>(Targets_Device,0x48)(Targets_Device,Targets_BackBuffer,nullptr,Rec(0x20)+8);}while(Targets_Retry);
    Targets_StoredRTV=R<void*>(Rec(0x20),8);
    if(Targets_NoDepth)W(Rec(0x20),16,Q(0));
    else{
        U desc[11]={Targets_SceneWidth,Targets_SceneHeight,1,1,0x2c,1,0,0,0x40,0,0};
        void*texture;
        do{Method<int(*)(void*,const U*,const void*,void**)>(Targets_Device,0x28)(Targets_Device,desc,nullptr,&texture);}while(Targets_Retry);
        U dsv[6]={0x2d,3,0,0,0,0};
        do{Method<int(*)(void*,void*,const U*,void*)>(Targets_Device,0x50)(Targets_Device,texture,dsv,Rec(0x20)+16);}while(Targets_Retry);
        Release(texture);
    }
    Relative(9,0,0x1b,0x60,true);
    if(Targets_NoDepth)return;
    Relative(8,0,0x1c,0xc0);
    if(Targets_NoDepth)return;
    Targets_Attach(0xb,Rec(0x60));
    Relative(2,0,0x29,0xe0);
    Duplicate(0x40,0x20,false);Duplicate(0xa0,0xc0,false);
    Relative(10,0,0x1c,0);
    Duplicate(0x80,0x60,true);
    Targets_ShadowSize=H(0x400u<<(U(IntValue(Targets_ShadowDvar))&31u));
    H shadow=Targets_ShadowSize;
    Targets_SpotSize=H(0x200u<<(U(IntValue(Targets_SpotDvar))&31u));
    H height=H(shadow*2);
    Sized(0,shadow,height,0x28,0,0x1c0);
    Targets_InitializeImage(R<void*>(Rec(0x1c0),0),3,0x28,shadow,height,1);
    H spot=H(Targets_SpotSize*2);
    Sized(1,spot,spot,0x28,0,0x1e0);
    Targets_InitializeImage(R<void*>(Rec(0x1e0),0),3,0x28,spot,spot,1);
    if(Targets_VendorObject)Targets_VendorMode(Targets_VendorObject,1);
    Relative(4,2,0x1c,0x180);Relative(5,2,0x1c,0x1a0);Relative(3,2,0x1c,0x140);
    Relative(6,2,0x1c,0x100);Relative(7,2,0x1c,0x120);
    Relative(0xd,0,0x29,0x5a0);
    for(U i=0;i<6;++i)Targets_CreateSlice(0xe+i,Rec(0x5c0+i*32),R<void*>(Rec(0x5a0),0),0x29,i);
    Relative(0x14,0,0x1c,0x680);Relative(0x15,0,0x1c,0x6a0);
    Relative(0x16,0,0x1d,0x6c0);Relative(0x17,0,0x1d,0x6e0);
    B*entry=Targets_Register(0x18,5,0);
    void*view=R<void*>(Rec(0x60),8),*resource;
    Method<void(*)(void*,void**)>(view,0x38)(view,&resource);
    U viewDesc[6];Method<void(*)(void*,U*)>(view,0x40)(view,viewDesc);
    U srv[6]={0x1d,viewDesc[1]==4?4u:viewDesc[1]==6?6u:0u,0,0xffffffffu,0,0};
    do{Method<int(*)(void*,void*,const U*,void*)>(Targets_Device,0x38)(Targets_Device,resource,srv,entry);}while(Targets_Retry);
    Release(resource);
    W(entry,24,R<H>(Rec(0x60),24));W(entry,26,R<H>(Rec(0x60),26));W(entry,28,H(1));
    Relative(0x27,1,0x22,0x320);Relative(0x28,1,0x22,0x340);
    Relative(0x29,0,10,0x360);
    for(U i=0;i<6;++i)Targets_CreateSlice(0x2a+i,Rec(0x380+i*32),R<void*>(Rec(0x360),0),10,i);
    Relative(0x30,0,0x18,0x440);
    float base=(float(H(Targets_DisplayWidth))/float(H(Targets_DisplayHeight)))*720.0f;
    int n=_mm_cvtt_ss2si(_mm_set_ss(base));
    if(n!=(-2147483647-1)&&float(n)!=base&&base<0.0f)--n;
    H width=H(n);
    for(U level=1;level<=4;++level){
        H w=H(width>>level),h=H(720u>>level);
        U id=0x31+(level-1)*2,off=0x460+(level-1)*64;
        Sized(id,w,h,10,1,off);Sized(id+1,w,h,10,1,off+32);
    }
    Relative(0x39,3,0x1c,0x560);
    if(Targets_VendorObject)Targets_VendorMode(Targets_VendorObject,0);
    Sized(0x3a,1024,32,0x18,1,0x580);
    Sized(0x26,256,256,0x1c,1,0x300);
    if(!R<H>(Rec(0x200),24)&&Targets_AuxEnabled)Targets_AllocateAux(Targets_AuxWidth,Targets_AuxHeight,Targets_AuxByte);
    if(Targets_DeferredEnabled&&!R<Q>(Rec(0x240),0))Targets_AllocateDeferred(Targets_DeferredArgs[0],Targets_DeferredArgs[1],Targets_DeferredArgs[2],Targets_DeferredArgs[3],Targets_DeferredArgs[4]);
    Relative(0x23,1,0x1c,0x700,true);Relative(0x24,1,0x1c,0x720);
    Targets_Attach(0x25,Rec(0x700));
}
}
#endif
