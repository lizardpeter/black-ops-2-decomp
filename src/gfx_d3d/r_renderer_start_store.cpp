// Original Store renderer start and backing resource setup.
#define T6_SOURCE "gfx_d3d/r_init.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_renderer_start{
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<B*>(p)+n,&v,sizeof v);}
extern "C"{
extern B *Start_Config,*Start_QueryBase,*Start_SourceInput,*Start_Source,*Start_CommandState,*Start_WindowRecords;
extern U Start_Timeout,Start_Ready,Start_Width,Start_Height,Start_ScreenWidth,Start_ScreenHeight,Start_TargetWidth,Start_TargetHeight;
extern H Start_TargetWidth16,Start_TargetHeight16;
extern Q Start_Viewport;
extern B Start_TargetReady,Start_BrightnessDirty;
extern float Start_Brightness;
extern int Start_WindowCount;
extern const B *Start_ModeDvar,*Start_RefreshDvar,*Start_FontResolutionDvar,*Start_ModeSideEffectDvar,*Start_BrightnessDvar;
extern void *Start_Context,*Start_CommandBuffer;
extern void* Start_Device;
extern volatile U Start_Retry;
int Start_Atoi(const char*);
void Start_SwapDesc(B*,const B*);
int Start_CreateDevice(void*,const B*,const B*);
void Start_MonitorMode();
void Start_WindowMode(const B*);
void Start_SetInt(const B*,int,U);
void Start_SetFloat(const B*,float,U);
void Start_SelectZones();
void Start_LoadZones();
void Start_UpdateVideo(const B*);
void Start_LockDevice();void Start_UnlockDevice();
void Start_InitializeSource(B*,const B*,U);
void Start_ContextTopology(void*,U);
void Start_InitializeCommandState(B*,void*);
void Start_SetTarget(B**,B);
void Start_InitializeTables();
void Start_InitializeDefaults();
extern B *Start_TempRecords,*Start_VertexBuffers,*Start_IndexBuffers,*Start_SlotRecords,*Start_ClientQueryPairs,*Start_ClientSlotIds;
extern const B* Start_BufferEnable;
extern B Start_Restart;
extern U Start_StreamHeight,Start_StreamCursor,Start_QueryIndex,Start_ClientCount;
extern B* Start_StreamImage;
extern void* Start_StreamTextures[2],*Start_GlobalQuery,*Start_FrameQueries[8],*Start_ClientQueries[4];
void Start_InitTargets();
void Start_InitSlots();
void Start_FormatTemp(char*,unsigned,const char*,const char*,int);
B* Start_CreateImage(const char*,U,U);
B* Start_RegisterTarget(int,B,B);
void Start_SetupImage(B*,U,U,U,U,U,void*);
void Start_InitBuffer(B*,U);
void Start_InitNoise();
void Start_LockSlots();void Start_UnlockSlots();
short Start_AllocateSlot(U,Q,B,float);
void Start_QueryPairs(B*);
void Start_ResetQueries();
extern B *Start_MeshPairs,*Start_FrameMesh,*Start_QuadMesh,*Start_QueryReferences,*Start_ViewPointers,*Start_ViewA,*Start_ViewB,*Start_ViewC;
extern U Start_VideoWidth,Start_VideoHeight;
void Start_InitMesh(B*,U,U,U);
void* Start_Allocate(U);
[[noreturn]] void Start_AllocationFailure();
void Start_QuadVertices(B*,float,float,float,float);
}
static int Int(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<int>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Start_Atoi(R<const char*>(d,0x20));
}
using CreateQuery=int(*)(void*,const Q*,void**);
using CreateResource=int(*)(void*,const void*,const void*,void**);
static int Query(const Q*desc,void**out){return reinterpret_cast<CreateQuery>((*static_cast<void***>(Start_Device))[0xc0/8])(Start_Device,desc,out);}
static void Resource(unsigned slot,const void*desc,void**out){do{reinterpret_cast<CreateResource>((*static_cast<void***>(Start_Device))[slot/8])(Start_Device,desc,nullptr,out);}while(Start_Retry);}
static void* Allocate(U n){void*p=Start_Allocate(n);if(!p)Start_AllocationFailure();return p;}
// 14071B300: meshes, paired queries, full-screen quads and per-view scratch links.
void InitializeMeshResources(){
    for(unsigned i=0;i<2;++i){B*p=Start_MeshPairs+i*0x40;Start_InitMesh(p,0xf000,0x6000,0x20);W(p,0x30,Allocate(0xc0000));}
    for(unsigned i=0;i<2;++i){
        B*p=Start_FrameMesh+i*0x4d6e00;W(p,0,Start_MeshPairs+i*0x40);
        Start_InitMesh(p+0x10,0x2400,0x1800,0x2c);
        W(p,0x40,Allocate(0x42000));W(p,0x40,Allocate(0x42000));
        for(unsigned group=0;group<4;++group)for(unsigned j=0;j<33;++j){
            B*dst=p+0x39befa+group*0x4a8+j*0x18;
            B*src=Start_QueryReferences+group*0x298+j*8;
            W(dst-18,0,R<Q>(src-0x108,0));W(dst-10,0,src);W(dst-2,0,H(group));W(dst,0,H(j));
        }
    }
    for(unsigned i=0;i<4;++i){
        B*p=Start_QuadMesh+i*0x50;Start_InitMesh(p+8,6,4,0x20);
        W(p-8,0,Q(0));W(p,0,float(Start_VideoWidth));W(p,4,float(Start_VideoHeight));
        Start_QuadVertices(p+8,0,0,float(Start_VideoWidth),float(Start_VideoHeight));
    }
    for(unsigned i=0;i<2;++i){B*p=Start_FrameMesh+i*0x4d6e00;if(R<int>(p,0x88))Start_InitBuffer(p+0x70,R<U>(p,0x74));}
    for(unsigned i=0;i<8;++i){
        B*p=Start_ViewPointers+i*0xc70;
        W(p-0x28,0,Start_ViewA+i*0xe98);W(p-8,0,Start_ViewB+i*0x658);W(p,0,Start_ViewC+i*0xf30);
    }
}
// 14071EA30: streaming staging images, dynamic buffers and original query pools.
bool InitializeDeviceResources(){
    Start_InitTargets();Start_InitSlots();
    for(unsigned i=0;i<32;++i){
        char name[72];Start_FormatTemp(name,64,"%s%d","streaming_temp_image_",int(i));
        B*image=Start_CreateImage(name,0,0);image[0x1f]=3;
        B*p=Start_TempRecords+i*16;W(p,0,image);W(p,8,0xffffffffu);W(p,12,0u);
    }
    if(!Start_Restart){
        InitializeMeshResources();
        Start_StreamImage=Start_RegisterTarget(0x19,4,1);
        Start_SetupImage(Start_StreamImage,0x200,Start_StreamHeight,4,10,0x1c,nullptr);
        U desc[9]={0x200,Start_StreamHeight,4,1,0x1c,3,0,0x10000,0};
        for(unsigned i=0;i<2;++i)Resource(0x30,desc,Start_StreamTextures+i);
        Start_StreamCursor=0;
    }
    for(unsigned i=0;i<2;++i)Start_InitBuffer(Start_VertexBuffers+i*0x18,0x1400000);
    for(unsigned i=0;i<2;++i){
        B*p=Start_IndexBuffers+i*0x18;W(p,0,0u);W(p,4,0xc000u);
        const B*d=Start_BufferEnable;
        if(d&&(R<int>(d,0x18)==1?d[0x20]!=0:Start_Atoi(R<const char*>(d,0x20))!=0)){
            U desc[6]={0x18000,2,2,0x10000,0,0};Resource(0x18,desc,reinterpret_cast<void**>(p+8));
        }
    }
    if(!Start_Restart)Start_InitNoise();
    Start_QueryIndex=0;const Q zero=0,one=1;
    if(Query(&zero,&Start_GlobalQuery)<0)return false;
    for(unsigned i=0;i<8;++i)if(Query(&zero,Start_FrameQueries+i)<0)return false;
    Start_LockSlots();
    for(unsigned i=0;i<40;++i)for(unsigned j=0;j<2;++j){
        B*p=Start_SlotRecords+i*0x38;void*q=nullptr;if(Query(&one,&q)<0)q=nullptr;
        p[0x20+j]=0;W(p,0x28+j*8,q);
    }
    Start_UnlockSlots();
    if(!Start_Restart){
        for(unsigned i=0;i<Start_ClientCount;++i)W(Start_ClientSlotIds+i*32,0,Start_AllocateSlot(i,0,0,0));
        for(unsigned i=0;i<Start_ClientCount;++i)for(unsigned j=0;j<32;++j)Start_QueryPairs(Start_ClientQueryPairs+i*0xf08+j*0x78);
        Start_ResetQueries();
        for(unsigned i=0;i<4;++i){void*q=nullptr;if(Query(&one,&q)<0)q=nullptr;Start_ClientQueries[i]=q;}
    }
    return true;
}
// 1407203B0: actual Store startup preserves the original forced 720 font zone.
bool InitializeRenderer(const B*input){
    std::memcpy(Start_Config,input,40);Start_Timeout=20;
    B desc[80];Start_SwapDesc(desc,input);
    if(Start_CreateDevice(R<void*>(input,0),desc,input)<0)return false;
    using SetupQuery=void(*)(void*,void*,void**);
    for(unsigned group=0;group<4;++group)for(unsigned i=0;i<33;++i)
        do{reinterpret_cast<SetupQuery>((*static_cast<void***>(Start_Device))[0xd8/8])(Start_Device,nullptr,reinterpret_cast<void**>(Start_QueryBase+(0xa4+group*0x53+i)*8));}while(Start_Retry);
    if(Int(Start_ModeDvar)==2){Start_MonitorMode();Start_WindowMode(input);}
    Start_SetInt(Start_RefreshDvar,R<int>(input,0x24),0);
    Start_SetInt(Start_FontResolutionDvar,720,0);
    Start_SelectZones();Start_LoadZones();Start_Ready=1;
    const B*d=Start_ModeSideEffectDvar;if(d&&R<int>(d,0x18)!=1)Start_Atoi(R<const char*>(d,0x20));
    Start_UpdateVideo(input);Start_ScreenWidth=Start_Width;Start_ScreenHeight=Start_Height;Start_Viewport=0;
    if(!InitializeDeviceResources())return false;
    Start_LockDevice();
    for(unsigned i=0;i<0xf30;++i)Start_SourceInput[i]=0;
    Start_InitializeSource(Start_Source,Start_SourceInput,0);
    W(Start_CommandState,0xd0,Start_Context);Start_ContextTopology(Start_Context,4);
    Start_InitializeCommandState(Start_CommandState,Start_CommandBuffer);
    Start_ScreenWidth=Start_Width;Start_ScreenHeight=Start_Height;Start_TargetWidth=Start_TargetWidth16;Start_TargetHeight=Start_TargetHeight16;Start_Viewport=0;Start_TargetReady=1;
    B*states[]={Start_Source,Start_CommandState};Start_SetTarget(states,2);Start_UnlockDevice();
    if(Start_Brightness<0.9f||Start_Brightness>0.9f){Start_SetFloat(Start_BrightnessDvar,0.9f,0);Start_Brightness=0.9f;Start_BrightnessDirty=1;}
    Start_InitializeTables();Start_InitializeDefaults();
    B*p=Start_WindowRecords+static_cast<long long>(Start_WindowCount)*32;
    W(p,0,R<Q>(input,0));W(p,24,R<U>(input,0x20));W(p,28,R<U>(input,0x24));++Start_WindowCount;
    return true;
}
}
#endif
