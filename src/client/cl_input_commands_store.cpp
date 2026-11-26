// Store MP x64 input command helpers. Original bodies: 140193FE0,
// 140198EE0, 1405F8550, 140192B70. External calls are explicit boundaries.
#define T6_SOURCE "client/cl_input.cpp"
#if defined(T6_STORE) && defined(T6_STORE_INPUT_REPLAY)
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>
namespace store_input_commands {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T> static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const Byte*>(p)+off,sizeof(v));return v;}
template<class T> static void W(void*p,unsigned off,T v){std::memcpy(static_cast<Byte*>(p)+off,&v,sizeof(v));}
extern "C" {
extern Byte *Command_Keyboard; // 143C0C790, 0x438 bytes/client
extern Byte *Command_Clients; // 140E61BF8
extern int Command_ClientCount,Command_Menu,Command_State; // 140E5CA9C,14212560C,140E61798
extern U32 Command_KeyCatchers; // 140E61794
void Command_CombinedButton(int,Byte*,int,int,int); // 140193ED0
extern Byte *Command_SpectatorUI; // 140E4F8F0, 0x4130/client
extern Byte *Command_DemoUI; // 1421214C0
extern Byte *Command_FocusStack; // 142123C80, 16 bytes/entry
extern int Command_FocusCount; // 142123D80
void* Command_FindMenu(void*,const char*); // 140419AC0
bool Command_MenuGate(int,void*); // 14042E5D0
void Command_ExecBinding(int,int,int,float); // 140197520
void Command_Animate(int,void*,void*,const char*,const char*,int,bool); // 1404194C0
extern const Byte *Command_Strafe,*Command_Camera; // 140E4F870,140E4F860
double Command_Atof(const char*); // IAT 1408A7DD8
int Command_Airburst(int,void*,Byte*,U32*,int); // 14018E200
}
struct Button {unsigned off,word;U32 mask;};
static void Consume(Byte*kb,Byte*cmd,Button b){
    if(kb[b.off]||kb[b.off+1])W(cmd,b.word,R<U32>(cmd,b.word)|b.mask);
    kb[b.off+1]=0;
}
void CmdButtons(int client,Byte*cmd){
    Byte*kb=Command_Keyboard+std::ptrdiff_t(client)*0x438;
    constexpr Button first[]={
        {0x1a8,4,0x80000000u},{0x1c0,4,0x40000},{0x1d8,4,0x20000},
        {0x208,8,0x10},{0x1f0,4,0x10000},{0x220,4,0x20000000},
        {0x238,4,0x10000000},{0x250,4,0x8000000},{0x268,4,0x4000000},
        {0x280,4,0x2000000},{0x298,4,0x1000000},{0x2b0,4,0x800000},
        {0x2c8,4,0x400000},{0x100,4,0x200000},{0x2e0,4,0x80}};
    for(auto b:first)Consume(kb,cmd,b);
    Command_CombinedButton(client,cmd+4,34,19,3);
    constexpr Button second[]={
        {0x358,4,0x200},{0x370,4,0x100},{0x388,4,8},
        {0x3a0,8,0x20000000},{0x3b8,8,0x10000000},{0x3d0,8,0x8000000},
        {0x3e8,8,0x4000000},{0x400,8,1},{0x418,8,0x2000000},{0x2f8,8,2}};
    for(auto b:second)Consume(kb,cmd,b);
    if(client!=-1&&Command_KeyCatchers&&Command_Menu!=8)W(cmd,4,R<U32>(cmd,4)|4);
    if(Command_State>6){
        const Byte*cl=client<Command_ClientCount?Command_Clients:nullptr;
        if(R<int>(cl,0x8c)==5)Consume(kb,cmd,{0x130,4,0x200000});
    }
}
bool DemoViewAngleGate(int client){
    if(client==-1)return false;
    void*menu=Command_FindMenu(Command_DemoUI,"demo_error");
    if(!menu)return false;
    for(int i=Command_FocusCount-1;i>=0;--i)
        if(R<void*>(Command_FocusStack,unsigned(i)*16)==menu)return Command_MenuGate(client,Command_DemoUI);
    return false;
}
bool SpectatorInput(int client,int key){
    int bind;const char*cycle;const char*slide;
    switch(key){
    case 4:case 0xca:bind=0x27;cycle="SpecViewToggle";slide=nullptr;break;
    case 5:case 0xc8:bind=0x25;cycle="CyclePrev";slide="StartRight";break;
    case 6:case 0xc9:bind=0x23;cycle="CycleNext";slide="StartLeft";break;
    default:return false;
    }
    Command_ExecBinding(client,bind,key,1.0f);
    Command_ExecBinding(client,bind+1,key,0.0f);
    void*ui=Command_SpectatorUI+std::ptrdiff_t(client)*0x4130;
    void*menu=Command_FindMenu(ui,"hud_spectator");
    Command_Animate(client,ui,menu,cycle,"Flash",1,false);
    if(slide){
        menu=Command_FindMenu(ui,"hud_spectator");
        Command_Animate(client,ui,menu,"SpectatorName","Flash",1,false);
        menu=Command_FindMenu(ui,"spectatingPlayerCard");
        Command_Animate(client,ui,menu,"spectatingPlayerSlide",slide,1,false);
    }
    return true;
}
static float DvarFloat(const Byte*d){
    if(!d)return 0.0f;
    const int type=R<int>(d,0x18);
    if(type==2)return R<float>(d,0x20);
    if(type==6)return float(R<int>(d,0x20));
    return float(Command_Atof(R<const char*>(d,0x20)));
}
void UpdateAirburstMarkDistance(int,void*ps,Byte*cmd,float forward,float right,float pitch,float yaw,float scale){
    const float strafe=DvarFloat(Command_Strafe),camera=DvarFloat(Command_Camera);
    const float inv=1.0f/scale;
    const float p=inv*pitch,y=inv*yaw,f=inv*forward,r=inv*right;
    const float cameraMotion=std::sqrt(y*y+p*p);
    const float strafeMotion=std::sqrt(r*r+f*f);
    U32 distance;
    // Store passes literal 1, not the incoming local-client argument.
    if(Command_Airburst(1,ps,cmd+4,&distance,strafeMotion>strafe||cameraMotion>camera)){
        W(cmd,0x38,std::uint16_t(distance));
        W(cmd,8,R<U32>(cmd,8)|0x1000);
    }
}
}
#endif
