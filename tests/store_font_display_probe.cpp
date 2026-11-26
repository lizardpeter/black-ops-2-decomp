#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/r_display_resources_store.cpp"
#include <cstdlib>
#define MS __attribute__((ms_abi))
namespace store_display{
extern "C"{
int Display_Width,Display_Height;
std::uint16_t Display_TargetWidth,Display_TargetHeight;
B Display_UI3DSlots[6][0xa4];
const B*Display_MSAADvar,*Display_AspectDvar,*Display_FullscreenDvar,*Display_BufferDvar;
Q Display_SampleDesc;
B Display_VideoConfig[0x34],Display_OverrideFullscreen;
U Display_OverrideWidth,Display_OverrideHeight;
void*Display_ClearDvar,*Display_WideDvar;
const char*Display_True="1",*Display_False="0";
int Display_Atoi(const char*s){return std::atoi(s);}
void Display_Clear(void*){}
void Display_SetDvar(void*,const void*,int){}
U Display_ClientCount;
B Display_ClientQueries[2][0xf08];
U Display_CreatePending;
B Display_ResourceSlots[40][0x38];
const char*Display_TargetNames[1];
B Display_TargetRecords[1][0x68];
void*Display_TargetHash[2048];
void*Display_DefaultTexture,*Display_TextureCache[16],*Display_Context;
B Display_SamplerByte[16];
int Display_SamplerCache[16];
void Display_BindDefaultTexture(void*,U,U,void*const*){}
void Display_BindSampler(void*,U,U){}
void Display_CreateBuffer(const void*,const void*,void*){}
int Display_CreateQuery(const Q*,Q*){return -1;}
}
static U trace[16],nt;
extern "C"{
void Display_AdjustRect(int*r,U s,int menu,U ext){trace[nt++]=s;trace[nt++]=U(menu);trace[nt++]=ext;r[0]-=5;r[1]-=20;r[2]+=5;r[3]+=5;}
void Display_SetStyle(void*,int k,int v){trace[nt++]=U(k);trace[nt++]=U(v);}
void Display_SetPosition(void*,void*,int x,int y,int w,int h,U flags){trace[nt++]=U(x);trace[nt++]=U(y);trace[nt++]=U(w);trace[nt++]=U(h);trace[nt++]=flags;}
void Display_LockResources(){trace[nt++]=1;}
void Display_UnlockResources(){trace[nt++]=2;}
}
static int MS MAtoi(const char*s){return Display_Atoi(s);}
static void MS MClear(void*,void*,int){}
static int MS MAdjust(int*r,U s,int m,U e){Display_AdjustRect(r,s,m,e);return 1;}
static int MS MStyle(void*h,int k,int v){Display_SetStyle(h,k,v);return 0;}
static int MS MPosition(void*h,void*a,int x,int y,int w,int ht,U f){Display_SetPosition(h,a,x,y,w,ht,f);return 1;}
static void MS MLock(void*){Display_LockResources();}
static void MS MUnlock(void*){Display_UnlockResources();}
static B dvar[48],in[64],desc[72];
struct Capture{B data[2*0xf08];Q sample;int result;U calls[16];};
extern "C"{
void*DisplayMock(int i){switch(i){case 0:return reinterpret_cast<void*>(MAtoi);case 1:return reinterpret_cast<void*>(MClear);case 2:return reinterpret_cast<void*>(MAdjust);case 3:return reinterpret_cast<void*>(MStyle);case 4:return reinterpret_cast<void*>(MPosition);case 5:return reinterpret_cast<void*>(MLock);default:return reinterpret_cast<void*>(MUnlock);}}
unsigned DisplaySize(){return sizeof(Capture);}
void DisplayRun(void*image,U op,U seed,void*out){
    B*b=static_cast<B*>(image);Capture cap{};nt=0;std::memset(trace,0,sizeof(trace));U st=seed+1;
    auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%4096)-2048)/1024.0f;};
    std::memset(dvar,0,sizeof(dvar));std::memset(in,0,sizeof(in));const B*d=dvar;U v=next()%20;
    switch(seed%6){case 0:d=nullptr;break;case 1:W(dvar,0x18,1);W(dvar,0x20,B(v&1));break;case 2:W(dvar,0x18,2);W(dvar,0x20,float(v)+0.25f);break;case 3:W(dvar,0x18,6);W(dvar,0x20,v);break;case 4:W(dvar,0x18,7);W(dvar,0x20,v);break;default:W(dvar,0x18,8);W(dvar,0x20,v&1?"16":"1");break;}
    W(in,0,Q(0x12345678));W(in,8,next());in[0xc]=B(next()&1);
    W(in,16,int(next()%2000)-1000);W(in,20,int(next()%2000)-1000);
    for(unsigned i=24;i<40;i+=4)W(in,i,next()%8000+1);
    if(op==0){
        Display_Width=int(next()%8000+1);Display_Height=int(next()%4000+1);Display_TargetWidth=std::uint16_t(next()%8000+1);Display_TargetHeight=std::uint16_t(next()%4000+1);U slot=next()%8;
        float x=f(),y=f(),w=f(),h=f();std::memset(Display_UI3DSlots,0xa5,sizeof(Display_UI3DSlots));
        if(b){W(b,0x30443fc,Display_Width);W(b,0x3044400,Display_Height);W(b,0x2c3f9c8,Display_TargetWidth);W(b,0x2c3f9ca,Display_TargetHeight);std::memcpy(b+0x2838700,Display_UI3DSlots,sizeof(Display_UI3DSlots));reinterpret_cast<void(MS*)(U,float,float,float,float)>(b+0x723a90)(slot,x,y,w,h);std::memcpy(cap.data,b+0x2838700,sizeof(Display_UI3DSlots));}
        else{SetUI3DViewport(slot,x,y,w,h);std::memcpy(cap.data,Display_UI3DSlots,sizeof(Display_UI3DSlots));}
    }else if(op==1){
        Display_MSAADvar=d;std::memset(desc,0xa5,72);
        if(b){W(b,0x27e7158,d);reinterpret_cast<void(MS*)(B*,const B*)>(b+0x71e360)(desc,in);cap.sample=R<Q>(b,0x282e538);}
        else{BuildSwapChainDesc(desc,in);cap.sample=Display_SampleDesc;}std::memcpy(cap.data,desc,72);
    }else if(op==2){
        Display_AspectDvar=d;Display_OverrideFullscreen=B(next()&1);Display_OverrideWidth=next()%8000+1;Display_OverrideHeight=next()%4000+1;Display_ClearDvar=nullptr;Display_WideDvar=nullptr;
        std::memset(Display_VideoConfig,0xa5,sizeof(Display_VideoConfig));W(Display_VideoConfig,0x28,1.6f);
        if(b){W(b,0x27e6f90,d);W(b,0x27f1dbc,Display_OverrideFullscreen);W(b,0x27f1dc0,Display_OverrideWidth);W(b,0x27f1dc4,Display_OverrideHeight);W(b,0x27e7100,Q(0));W(b,0x1f037d8,Q(0));std::memcpy(b+0x282e5c0,Display_VideoConfig,sizeof(Display_VideoConfig));reinterpret_cast<void(MS*)(const B*)>(b+0x71e440)(in);std::memcpy(cap.data,b+0x282e5c0,sizeof(Display_VideoConfig));}
        else{UpdateVideoConfig(in);std::memcpy(cap.data,Display_VideoConfig,sizeof(Display_VideoConfig));}
    }else if(op==3){
        Display_FullscreenDvar=d;
        if(b){W(b,0x27e7230,d);reinterpret_cast<void(MS*)(const B*)>(b+0x7206e0)(in);}else SetWindowMode(in);
    }else if(op==4){
        Display_ClientCount=next()%3;std::memset(Display_ClientQueries,0xa5,sizeof(Display_ClientQueries));
        if(b){W(b,0x27e73a8,Display_ClientCount);std::memcpy(b+0x2838af0,Display_ClientQueries,sizeof(Display_ClientQueries));reinterpret_cast<void(MS*)()>(b+0x724f40)();std::memcpy(cap.data,b+0x2838af0,sizeof(Display_ClientQueries));}
        else{ResetClientQueries();std::memcpy(cap.data,Display_ClientQueries,sizeof(Display_ClientQueries));}
    }else{
        std::memset(Display_ResourceSlots,0xa5,sizeof(Display_ResourceSlots));U used=next()%41;for(unsigned i=used;i<40;++i)Display_ResourceSlots[i][0]=0;U a=next();B k=B(next());float c=f();
        if(b){std::memcpy(b+0x2c3edd0,Display_ResourceSlots,sizeof(Display_ResourceSlots));cap.result=reinterpret_cast<short(MS*)(U,Q,B,float)>(b+0x740be0)(a,0,k,c);std::memcpy(cap.data,b+0x2c3edd0,sizeof(Display_ResourceSlots));}
        else{cap.result=AllocateResourceSlot(a,0,k,c);std::memcpy(cap.data,Display_ResourceSlots,sizeof(Display_ResourceSlots));}
    }
    std::memcpy(cap.calls,trace,sizeof(trace));std::memcpy(out,&cap,sizeof(cap));
}
}
}
