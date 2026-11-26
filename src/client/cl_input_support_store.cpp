// Exact Store MP input support bodies recovered from the pinned x64 PE.
// 1400F1E90, 1401A3350, 14049A0A0.
#define T6_SOURCE "client/cl_input.cpp"
#if defined(T6_STORE) && defined(T6_STORE_INPUT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_input_support {
using Byte=unsigned char;using U32=std::uint32_t;using U64=std::uint64_t;
struct Dvar;
extern "C" {
extern int Input_DemoMode; // 1425A3308
extern Byte Input_PlaybackInited; // 142653908
extern const Byte *Input_Playback; // 142653900
extern const Byte *Input_Cgame; // 140E53A20
extern U32 Input_KeyCatchers; // 140E61794
extern int Input_Menu; // 14212560C
bool Input_DemoBlocksAngles(int); // 1405F8550
extern Byte *Input_Clients; // 140E61BF8
extern int Input_ClientCount; // 140E5CA9C
extern const Byte *Input_LastInput; // array starts 141EF91F4, stride 24
extern const Dvar *Input_ClickToContinue; // 141F02EF8
extern const Dvar *Input_MouseEnabled; // 1421E2468
extern U64 Input_Window; // 1421E4520
extern Byte Input_MouseInitialized,Input_MouseActive; // 14420E665,14420E664
extern int Input_AppActive; // 14420D61C
extern int Input_ShowCursor; // 1421E2460
int Input_Atoi(const char*); // IAT 1408A7DC0
int Input_PostMessage(U64,U32,U64,U64); // IAT 1408A7960
int Input_Show(int); // IAT 1408A78F8
U64 Input_Foreground(); // IAT 1408A78E8
void Input_MouseMove(); // 140499A10
void Input_GamepadFrame(); // 140499460
}
template<class T>static T R(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static float F(U32 u){float f;std::memcpy(&f,&u,4);return f;}
static bool Bool(const Dvar*d){
    if(!d)return false;
    if(R<int>(d,0x18)==1)return R<Byte>(d,0x20)!=0;
    return Input_Atoi(R<const char*>(d,0x20))!=0;
}
// 40 original instructions. The demo path can block even with no catcher.
bool ShouldUpdateViewAngles(int client){
    if(Input_DemoMode==2){
        if(Input_PlaybackInited && (R<Byte>(Input_Playback,0x14)||R<int>(Input_Cgame,0x14)))return false;
        if(Input_DemoBlocksAngles(client))return false;
    }
    return client==-1||!Input_KeyCatchers||Input_Menu==8;
}
// 42 instructions; exact short-angle truncation, rather than round-to-nearest.
void FinishMove(int client,Byte*cmd){
    Byte*cl=client<Input_ClientCount?Input_Clients:nullptr;
    for(unsigned i=0;i<3;++i)W(cmd,0x18+4*i,R<U32>(cl,0x2c00+4*i));
    W(cmd,0,R<U32>(cl,0x2abc));
    for(unsigned i=0;i<3;++i){
        const float a=(R<float>(cl,0x2ca4+4*i)+R<float>(cl,0x2c18+4*i))*F(0x43360b61);
        const int n=_mm_cvttss_si32(_mm_set_ss(a));
        W(cmd,0xc+4*i,U32(n)&0xffffu);
    }
    W(cmd,0x3a,R<std::uint16_t>(Input_LastInput,client*24));
    W(cmd,4,R<U32>(cmd,4)|R<U32>(cl,0x2c98));
    W(cmd,8,R<U32>(cmd,8)|R<U32>(cl,0x2c9c));
    W(cl,0x2c98,U64(0));
}
// 67 instructions; Store's polling path differs from the older reference.
void InputFrame(){
    if(Bool(Input_ClickToContinue))Input_PostMessage(Input_Window,0x201,1,0);
    if(!Input_MouseInitialized)return;
    if(!Input_AppActive){
        if(Input_MouseActive){
            Input_MouseActive=0;Input_ShowCursor=1;
            int count=Input_Show(1);
            while(count)count=Input_Show(count<0);
        }
        return;
    }
    if(Bool(Input_MouseEnabled)){
        if(!Input_MouseActive){
            const U64 foreground=Input_Foreground();
            Input_MouseActive=1;
            if(foreground!=Input_Window)Input_MouseActive=0;
        }
    }else Input_MouseActive=0;
    Input_MouseMove();
    if(Input_Foreground()==Input_Window)Input_GamepadFrame();
}
}
#endif
