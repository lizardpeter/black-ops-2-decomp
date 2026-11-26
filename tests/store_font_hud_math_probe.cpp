#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/cgame/cg_hud_math_store.cpp"
#include <cstdlib>
#include <initializer_list>
#define MS __attribute__((ms_abi))
namespace store_hud_math{
static B state[0x85000],clients[72],placements[4*0x7c],dvars[6][48],elem[64];
extern "C"{
B*Hud_State=state;
const B*Hud_Clients=clients,*Hud_Placements=placements+0x7c;
const B*Hud_ColorEnable,*Hud_ColorScale,*Hud_EdgeEnable,*Hud_EdgeRadius,*Hud_FadeRadius,*Hud_FadeTime;
int Hud_Atoi(const char*s){return std::atoi(s);}
double Hud_Atof(const char*s){return std::atof(s);}
}
static int MS MAtoi(const char*s){return Hud_Atoi(s);}
static double MS MAtof(const char*s){return Hud_Atof(s);}
struct Capture{float values[12];int result,timestamp;};
extern "C"{
void*HudMock(unsigned i){return i?reinterpret_cast<void*>(MAtof):reinterpret_cast<void*>(MAtoi);}
unsigned HudSize(){return sizeof(Capture);}
void HudRun(void*image,unsigned op,unsigned seed,void*out){
    auto*b=static_cast<B*>(image);Capture cap{};
    U st=seed+1;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%8192)-4096)*0.125f;};
    auto dvar=[&](unsigned i,int t,int value)->const B*{
        B*p=dvars[i];std::memset(p,0,48);
        if(t==0)return nullptr;
        W(p,0x18,t==1?1:t==2?2:t==3?6:t==4?7:8);
        if(t==1)W(p,0x20,B(value!=0));
        else if(t==2)W(p,0x20,float(value)*0.125f);
        else if(t==3||t==4)W(p,0x20,value);
        else W(p,0x20,value&1?"1":"0");
        return p;
    };
    Hud_ColorEnable=dvar(0,seed%6,int(next()%15)-7);
    Hud_ColorScale=dvar(1,2+next()%2,int(next()%25)-12);
    Hud_EdgeEnable=dvar(2,seed%2+1,1); // bool or string only; override type 2 below
    if((seed&1)!=0)Hud_EdgeEnable=dvar(2,5,1);
    Hud_EdgeRadius=dvar(3,2,int(next()%32));
    Hud_FadeRadius=dvar(4,3,int(next()%800));
    Hud_FadeTime=dvar(5,seed%6,int(next()%1000));
    if(seed&0x400)Hud_EdgeEnable=nullptr;
    int client=int(next()%3);
    for(int i=0;i<3;++i){W(clients,i*24,(seed&(1u<<(i+8)))?i:99);W(clients,i*24+8,int(next()%3));}
    for(unsigned i=0;i<4;++i){
        B*p=placements+i*0x7c;float w=float(next()%2000+320),h=float(next()%1000+240);
        W(p,0,1.0f);W(p,4,1.25f);W(p,0x38,f());W(p,0x3c,f());W(p,0x40,w);W(p,0x44,h);
        W(p,0x60,20.0f);W(p,0x64,24.0f);W(p,0x68,w-20.0f);W(p,0x6c,h-24.0f);
    }
    for(unsigned off:{0x4d924u,0x64918u})for(unsigned i=0;i<12;i+=4)W(state,off+i,f());
    for(unsigned off:{0x4d934u,0x64924u})for(unsigned i=0;i<36;i+=4)W(state,off+i,f()*0.001953125f);
    W(state,0x4d910,1.5f);W(state,0x4d914,0.75f);W(state,0x64948,1.25f);W(state,0x6494c,0.5f);
    W(state,0x48288,float(next()%4)*0.5f);W(state,0x4808c,int(next()%2000));W(state,0x6b770,int(next()%2000)*(seed&1));
    elem[0x1b]=B(next()>>24);
    float pos[3]={f(),f(),f()},q[8];for(float&v:q)v=f()*0.001953125f;
    for(float&v:cap.values)v=f();
    B color[4];for(auto&v:color)v=B(next()>>24);
    float left=float(next()%30),right=float(next()%30),top=float(next()%30),bottom=float(next()%30);
    if(b){
        W(b,0xe53a20,Hud_State);std::memcpy(b+0x1ef91e4,clients,sizeof(clients));std::memcpy(b+0xe63390-0x7c,placements,sizeof(placements));
        W(b,0x1f04888,Hud_ColorEnable);W(b,0x3bfb098,Hud_ColorScale);W(b,0xd76628,Hud_EdgeEnable);W(b,0xd76590,Hud_EdgeRadius);W(b,0xd76650,Hud_FadeRadius);W(b,0xd76640,Hud_FadeTime);
        switch(op){
        case 0:reinterpret_cast<void(MS*)(const B*,float*)>(b+0xf3d70)(color,cap.values);break;
        case 1:cap.result=reinterpret_cast<bool(MS*)(int,const float*,float*)>(b+0xf4c00)(client,pos,cap.values);break;
        case 2:cap.result=reinterpret_cast<bool(MS*)(int,const float*,float*)>(b+0xf4e90)(client,pos,cap.values);break;
        case 3:cap.values[0]=pos[0];cap.values[1]=pos[1];cap.result=reinterpret_cast<bool(MS*)(int,float*,float,float,float,float,float*,float*)>(b+0xf5120)(client,cap.values,left,right,top,bottom,cap.values+2,cap.values+4);break;
        case 4:reinterpret_cast<void(MS*)(int,const B*,const float*,float*)>(b+0xf5510)(client,elem,pos,cap.values);break;
        case 5:reinterpret_cast<void(MS*)(const float*,float*)>(b+0xde970)(q,cap.values);break;
        }
    }else switch(op){
        case 0:UnpackHudColor(color,cap.values);break;
        case 1:cap.result=WorldToHud(client,pos,cap.values);break;
        case 2:cap.result=WorldToHudAlternate(client,pos,cap.values);break;
        case 3:cap.values[0]=pos[0];cap.values[1]=pos[1];cap.result=ClampHud(client,cap.values,left,right,top,bottom,cap.values+2,cap.values+4);break;
        case 4:FadeHudNearCenter(client,elem,pos,cap.values);break;
        case 5:QuaternionAxis(q,cap.values);break;
    }
    cap.timestamp=R<int>(state,0x6b770);std::memcpy(out,&cap,sizeof(cap));
}
}
}
