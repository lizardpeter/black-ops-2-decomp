// Original Store MP matrix cache and monitor-selection bodies.
#define T6_SOURCE "gfx_d3d/r_init.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_matrix_display{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof(v));}
extern "C"{
void Matrix_Base8(B*,B*,U);
void Matrix_Base12(B*,B*,U);
void Matrix_Base16(B*,B*,U);
void Matrix_Base20(B*,B*,U);
void Matrix_Base28(B*,B*);
void Matrix_Invert(const B*,B*); // adapter for original 140235700 XMM0-XMM3 register convention
extern const B* Monitor_IndexDvar;
extern void* Monitor_Device,*Monitor_SwapChain,*Monitor_CurrentDvar,*Monitor_ResolutionDvar;
extern const void*Monitor_DxgiDeviceIID,*Monitor_AdapterIID;
extern int Monitor_FallbackWidth,Monitor_FallbackHeight;
extern const char*Monitor_ResolutionNames[];
int Monitor_Atoi(const char*);
int Monitor_SystemMetrics(int);
void Monitor_QueryInterface(void*,const void*,void**);
void Monitor_GetAdapter(void*,const void*,void**);
int Monitor_EnumOutputs(void*,U,void**);
void Monitor_GetOutputDesc(void*,void*);
void Monitor_Release(void*);
void Monitor_GetInfo(void*,void*);
void Monitor_SetDvar(void*,U,int);
void Monitor_FormatResolution(char*,unsigned,int,int);
void Monitor_SetFullscreen(void*,int,void*);
void Monitor_GetContainingOutput(void*,void**);
void Monitor_Refresh();
extern U Sync_MainThread,Sync_ThreadID;
extern B Sync_Enabled;
extern volatile U Sync_Nesting,Sync_Ack;
extern U Sync_Mode,Sync_Override,Sync_WaitCount;
extern void*Sync_Event;
U*Sync_TLSID();
U Sync_CurrentThread();
void Sync_SetEvent(void*);
void Sync_Sleep(U);
void Sync_Pump();
void Sync_Service();
}
static B*Align16(B*p){return reinterpret_cast<B*>(reinterpret_cast<Q>(p)&~Q(15));}
static void Transpose(B*d,const B*s){
    U m[16];std::memcpy(m,s,64);for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)W(Align16(d+i*16),j*4,m[j*4+i]);
}
// 140765F10
void UpdateMatrixBase(B*s,B*base,U index){
    unsigned src=0,dst=0,stamp=0,revision=0;
    switch(index){
        case 4:src=0x1730;dst=0x100;stamp=0x1a6e;revision=0x1aa8;break;
        case 8:Matrix_Base8(s,base,index);return;
        case 12:Matrix_Base12(s,base,index);return;
        case 16:Matrix_Base16(s,base,index);return;
        case 20:Matrix_Base20(s,base,index);return;
        case 24:src=0x1880;dst=0x600;stamp=0x1a96;revision=0x1ab2;break;
        case 28:Matrix_Base28(s,base+index*64);return;
        default:return;
    }
    std::memcpy(s+dst,s+src,64);
    float row[4];float v[4];std::memcpy(v,s+0x1ac0,16);
    for(unsigned i=0;i<4;++i)row[i]=((v[2]*R<float>(s,dst+32+i*4)+v[3]*R<float>(s,dst+48+i*4))+v[1]*R<float>(s,dst+16+i*4))+v[0]*R<float>(s,dst+i*4);
    std::memcpy(Align16(s+dst+48),row,16);W(s,stamp,R<H>(s,revision));
}
// 1407660F0
B* GetMatrixConstant(B*s,U code,U row){
    U n=code-0xd3,base=n&~3u;H revision=R<H>(s,0x1aa6+(n>>2)*2);
    if(R<H>(s,0x18c0+code*2)!=revision){
        if(R<H>(s,0x18c0+(base+0xd3)*2)!=revision){UpdateMatrixBase(s,s,base);if(n==base)return s+n*64+row*16;}
        W(s,0x18c0+code*2,revision);
        B*dest=s+n*64;
        if(R<H>(s,0x18c0+((n^2)+0xd3)*2)==revision)Transpose(dest,s+(n^2)*64);
        else{
            U other=n^1;B*input=s+other*64;
            if(R<H>(s,0x18c0+(other+0xd3)*2)!=revision){Transpose(input,s+base*64);W(s,0x18c0+(other+0xd3)*2,revision);}
            B inv[64];Matrix_Invert(input,inv);std::memcpy(Align16(dest),inv,64);
        }
    }
    return s+n*64+row*16;
}
static U DvarIndex(){
    const B*d=Monitor_IndexDvar;if(!d)return 0;
    int t=R<int>(d,0x18);if(U(t-6)<2)return R<U>(d,0x20);if(t==1)return d[0x20]!=0;if(t==2)return U(_mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20))));return U(Monitor_Atoi(R<const char*>(d,0x20)));
}
// 14071F190
void MonitorRect(int*x,int*y,int*w,int*h){
    U index=DvarIndex();void*out=nullptr;
    if(!Monitor_Device){*x=0;*y=0;*w=Monitor_SystemMetrics(0);*h=Monitor_SystemMetrics(1);return;}
    void*dxgi=nullptr,*adapter=nullptr;
    Monitor_QueryInterface(Monitor_Device,Monitor_DxgiDeviceIID,&dxgi);
    Monitor_GetAdapter(dxgi,Monitor_AdapterIID,&adapter);
    Monitor_EnumOutputs(adapter,int(index-1)<0?0:index-1,&out);
    if(!out){Monitor_SetDvar(Monitor_CurrentDvar,0,0);*x=0;*y=0;*w=Monitor_FallbackWidth;*h=Monitor_FallbackHeight;}
    else{alignas(8) B desc[96];Monitor_GetOutputDesc(out,desc);Monitor_Release(out);B info[40];W(info,0,U(40));Monitor_GetInfo(R<void*>(desc,88),info);*x=R<int>(info,4);*y=R<int>(info,8);*w=R<int>(info,12)-*x;*h=R<int>(info,16)-*y;}
    Monitor_Release(adapter);Monitor_Release(dxgi);
}
// 14071F3E0
void SelectDesktopResolution(){
    int x,y,w,h;MonitorRect(&x,&y,&w,&h);char s[72];Monitor_FormatResolution(s,64,w,h);
    for(U i=0;Monitor_ResolutionNames[i];++i){const char*a=s,*b=Monitor_ResolutionNames[i];while(*a==*b&&*b){++a;++b;}if(*a==*b){Monitor_SetDvar(Monitor_ResolutionDvar,i,0);return;}}
}
// 140720E00
void SetFullscreenOutput(){
    U index=DvarIndex();void*dxgi=nullptr,*adapter=nullptr,*out=nullptr;
    Monitor_QueryInterface(Monitor_Device,Monitor_DxgiDeviceIID,&dxgi);Monitor_GetAdapter(dxgi,Monitor_AdapterIID,&adapter);
    if(index)Monitor_EnumOutputs(adapter,index-1,&out);
    Monitor_SetFullscreen(Monitor_SwapChain,1,out);
    if(!out){
        void*containing=nullptr;Monitor_GetContainingOutput(Monitor_SwapChain,&containing);index=0;
        int hr=Monitor_EnumOutputs(adapter,0,&out);
        while(hr!=int(0x887a0002u)){++index;if(out==containing)break;Monitor_Release(out);hr=Monitor_EnumOutputs(adapter,index,&out);}
        Monitor_Release(containing);
    }
    Monitor_SetDvar(Monitor_CurrentDvar,index,0);if(out)Monitor_Release(out);Monitor_Release(adapter);Monitor_Release(dxgi);Monitor_Refresh();
}
static bool SyncAllowed(){
    U*p=Sync_TLSID();U id=*p;if(!id){id=Sync_CurrentThread();*Sync_TLSID()=id;}
    return id==Sync_MainThread&&Sync_Enabled&&(Sync_Mode!=10||Sync_Override);
}
// 14071E160
void BeginRendererSync(){if(!SyncAllowed())return;U n=Sync_Nesting;Sync_Nesting=n+1;if(!n)Sync_SetEvent(Sync_Event);}
// 14071E200
void EndRendererSync(){
    if(!SyncAllowed())return;
    if(Sync_Nesting!=1){Sync_Nesting=Sync_Nesting-1;return;}
    while(!Sync_Ack){Sync_Sleep(1);Sync_Pump();}
    Sync_Ack=0;Sync_Nesting=Sync_Nesting-1;
    while(!Sync_Ack){++Sync_WaitCount;Sync_Sleep(1);Sync_Service();Sync_Pump();--Sync_WaitCount;}Sync_Ack=0;
}
}
#endif
