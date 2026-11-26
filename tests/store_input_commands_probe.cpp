#define T6_STORE
#define T6_STORE_INPUT_REPLAY
#include "../src/client/cl_input_commands_store.cpp"
#include <cassert>
#include <cstdlib>
#include <array>
#define MS __attribute__((ms_abi))
namespace store_input_commands {
using U=std::uint64_t;
static Byte keyboard[3*0x438],clients[0x100],ui[2*0x4130],demo[16],focus[64],cmd[60],ps[16],dvar[2][48];
static Byte* imageBase;static unsigned key;static unsigned lookupCount;
struct Trace{U count;U events[20][8];Byte keyboard[3*0x438];Byte cmd[60];U ret;};
static Trace trace;
static void Log(U tag,U a=0,U b=0,U c=0,U d=0,U e=0,U f=0,U g=0){
    assert(trace.count<20);auto &v=trace.events[trace.count++];v[0]=tag;v[1]=a;v[2]=b;v[3]=c;v[4]=d;v[5]=e;v[6]=f;v[7]=g;
}
static U Hash(const char*s){U v=0;for(;*s;++s)v=v*131+Byte(*s);return v;}
static U Context(void*p){
    if(p==Command_DemoUI || (imageBase&&p==imageBase+0x21214c0))return 9;
    auto*base=imageBase?imageBase+0xe4f8f0:Command_SpectatorUI;
    return U(static_cast<Byte*>(p)-base);
}
extern "C" {
Byte *Command_Keyboard=keyboard+0x438,*Command_Clients=clients;
int Command_ClientCount,Command_Menu,Command_State;U32 Command_KeyCatchers;
Byte *Command_SpectatorUI=ui,*Command_DemoUI=demo,*Command_FocusStack=focus;int Command_FocusCount;
const Byte *Command_Strafe,*Command_Camera;
void Command_CombinedButton(int c,Byte*b,int x,int y,int z){
    Log(1,U32(c),R<U32>(b,0),R<U32>(b,4),x,y,z);
    W(b,0,R<U32>(b,0)|0x800u);
    // Observe that the second batch has not yet consumed this latch.
    auto*kb=(imageBase?imageBase+0x3c0c790:Command_Keyboard)+std::ptrdiff_t(c)*0x438;
    Log(8,kb[0x1a9],kb[0x359]);kb[0x359]=1;
}
void* Command_FindMenu(void*p,const char*s){
    Log(2,Context(p),Hash(s));
    if(key&1)return nullptr;
    return reinterpret_cast<void*>(U(0x12340+16*lookupCount++));
}
bool Command_MenuGate(int c,void*p){Log(3,U32(c),Context(p));return key&2;}
void Command_ExecBinding(int c,int b,int k,float v){U32 bits;std::memcpy(&bits,&v,4);Log(4,U32(c),b,k,bits);}
void Command_Animate(int c,void*p,void*m,const char*a,const char*b,int n,bool f){Log(5,U32(c),Context(p),reinterpret_cast<U>(m),Hash(a),Hash(b),n,f);}
double Command_Atof(const char*s){Log(6,Hash(s));return std::atof(s);}
int Command_Airburst(int c,void*p,Byte*b,U32*out,int cancel){assert(p==ps);Log(7,U32(c),R<U32>(b,0),R<U32>(b,4),U32(cancel));*out=0xabc00000u|key;return key&2;}
}
static void MS MCombined(int c,Byte*b,int x,int y,int z){Command_CombinedButton(c,b,x,y,z);}
static void* MS MFind(void*p,const char*s){return Command_FindMenu(p,s);}
static bool MS MGate(int c,void*p){return Command_MenuGate(c,p);}
static void MS MExec(int c,int b,int k,float v){Command_ExecBinding(c,b,k,v);}
static void MS MAnimate(int c,void*p,void*m,const char*a,const char*b,int n,bool f){Command_Animate(c,p,m,a,b,n,f);}
static double MS MAtof(const char*s){return Command_Atof(s);}
static int MS MAir(int c,void*p,Byte*b,U32*out,int cancel){return Command_Airburst(c,p,b,out,cancel);}
extern "C" {
void* CommandsMock(unsigned i){void*m[]={reinterpret_cast<void*>(MCombined),reinterpret_cast<void*>(MFind),reinterpret_cast<void*>(MGate),reinterpret_cast<void*>(MExec),reinterpret_cast<void*>(MAnimate),reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MAir)};assert(i<7);return m[i];}
unsigned CommandsSize(){return sizeof(Trace);}
void CommandsRun(void*image,unsigned op,unsigned seed,void*out){
    key=seed;lookupCount=0;trace={};imageBase=static_cast<Byte*>(image);
    unsigned state=seed+1;auto next=[&](){state=state*1664525u+1013904223u;return state;};
    for(auto&v:keyboard){unsigned n=next()>>24;v=Byte(n%4==0?n:0);}
    for(auto&v:cmd)v=Byte(next()>>24);
    Command_ClientCount=2;Command_State=int((key>>2)&15);Command_KeyCatchers=(key&0x20)?8:0;Command_Menu=(key&0x40)?8:0;
    W(clients,0x8c,(key&0x80)?5:3);
    const int client=int((key>>8)%3)-1;
    Command_FocusCount=int((key>>10)%5);
    for(int i=0;i<4;++i)W(focus,i*16,reinterpret_cast<void*>(U((key&(1u<<(13+i)))?0x12340:0x99990)));
    const Byte**slots[]={&Command_Strafe,&Command_Camera};
    for(unsigned i=0;i<2;++i){
        unsigned type=(key>>(17+i*2))&3;
        *slots[i]=type?dvar[i]:nullptr;W(dvar[i],0x18,type==1?2:type==2?6:8);
        if(type==1)W(dvar[i],0x20,float((key>>i)&7)*0.5f);
        else if(type==2)W(dvar[i],0x20,int((key>>i)&7)-3);
        else W(dvar[i],0x20,(key&4)?"2.25":"-0.5");
    }
    const int keys[]={4,5,6,0xc8,0xc9,0xca,0,1,7,-1,0xcb};
    const int k=keys[key%11];const int sc=client<0?0:client;
    const float values[]={-100,-3,-0.5f,0,0.5f,3,100};
    float f=values[next()%7],r=values[next()%7],p=values[next()%7],y=values[next()%7],s=values[next()%7];
    if(image){
        auto*b=imageBase;
        std::memcpy(b+0x3c0c790-0x438,keyboard,sizeof(keyboard));
        W(b,0xe61bf8,Command_Clients);W(b,0xe5ca9c,Command_ClientCount);W(b,0xe61798,Command_State);
        W(b,0xe61794,Command_KeyCatchers);W(b,0x212560c,Command_Menu);
        W(b,0x2123d80,Command_FocusCount);std::memcpy(b+0x2123c80,focus,sizeof(focus));
        W(b,0xe4f870,Command_Strafe);W(b,0xe4f860,Command_Camera);
        switch(op){
        case 0:reinterpret_cast<void(MS*)(int,Byte*)>(b+0x193fe0)(client,cmd);break;
        case 1:trace.ret=reinterpret_cast<bool(MS*)(int)>(b+0x5f8550)(client);break;
        case 2:trace.ret=reinterpret_cast<bool(MS*)(int,int)>(b+0x198ee0)(sc,k);break;
        case 3:reinterpret_cast<void(MS*)(int,void*,Byte*,float,float,float,float,float)>(b+0x192b70)(client,ps,cmd,f,r,p,y,s);break;
        }
        std::memcpy(keyboard,b+0x3c0c790-0x438,sizeof(keyboard));
    }else switch(op){
        case 0:CmdButtons(client,cmd);break;
        case 1:trace.ret=DemoViewAngleGate(client);break;
        case 2:trace.ret=SpectatorInput(sc,k);break;
        case 3:UpdateAirburstMarkDistance(client,ps,cmd,f,r,p,y,s);break;
    }
    std::memcpy(trace.keyboard,keyboard,sizeof(keyboard));std::memcpy(trace.cmd,cmd,sizeof(cmd));std::memcpy(out,&trace,sizeof(trace));
}
}
}
