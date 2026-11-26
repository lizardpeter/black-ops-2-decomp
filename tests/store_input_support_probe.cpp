#define T6_STORE
#define T6_STORE_INPUT_REPLAY
#include "../src/client/cl_input_support_store.cpp"
#include <cassert>
#include <cstdlib>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_input_support {
struct Dvar{Byte bytes[104];};
static Byte cg[32],play[32],cl[0x2cb0],last[24],cmd[60];
static Dvar dvars[2];static unsigned key;static int cursorCount;
struct Trace{U count;U events[16][5];U out[20];};static Trace trace;
static void Push(U k,U a=0,U b=0,U c=0,U d=0){assert(trace.count<16);auto*p=trace.events[trace.count++];p[0]=k;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
extern "C" {
int Input_DemoMode;Byte Input_PlaybackInited;const Byte *Input_Playback,*Input_Cgame;
U32 Input_KeyCatchers;int Input_Menu;Byte *Input_Clients;int Input_ClientCount;const Byte *Input_LastInput;
const Dvar *Input_ClickToContinue,*Input_MouseEnabled;U64 Input_Window;Byte Input_MouseInitialized,Input_MouseActive;
int Input_AppActive,Input_ShowCursor;
bool Input_DemoBlocksAngles(int c){Push(1,U32(c));return key&32;}
int Input_Atoi(const char*s){int n=std::atoi(s);Push(2,U32(n));return n;}
int Input_PostMessage(U64 w,U32 m,U64 a,U64 b){Push(3,w,m,a,b);return 1;}
int Input_Show(int b){Push(4,b);return cursorCount+=b?1:-1;}
U64 Input_Foreground(){Push(5);return (key&64)?Input_Window:0x5678;}
void Input_MouseMove(){Push(6);}
void Input_GamepadFrame(){Push(7);}
}
static bool MS MDemo(int c){return Input_DemoBlocksAngles(c);}
static int MS MAtoi(const char*s){return Input_Atoi(s);}
static int MS MPost(U64 w,U32 m,U64 a,U64 b){return Input_PostMessage(w,m,a,b);}
static int MS MShow(int b){return Input_Show(b);}
static U64 MS MForeground(){return Input_Foreground();}
static void MS MMouse(){Input_MouseMove();}
static void MS MPads(){Input_GamepadFrame();}
extern "C" {
void*InputMock(unsigned i){void*m[]={reinterpret_cast<void*>(MDemo),reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MPost),reinterpret_cast<void*>(MShow),reinterpret_cast<void*>(MForeground),reinterpret_cast<void*>(MMouse),reinterpret_cast<void*>(MPads)};assert(i<7);return m[i];}
unsigned InputSize(){return sizeof(Trace);}
void InputRun(void*image,unsigned op,U32 seed,U32 a,U32 b,U32 c,void*out){
    key=seed;trace={};Input_DemoMode=(key&1)?2:0;Input_PlaybackInited=(key>>1)&1;
    Input_Playback=play;Input_Cgame=cg;W(play,0x14,Byte((key>>2)&1));W(cg,0x14,int((key>>3)&1));
    Input_KeyCatchers=(key&16)?8:0;Input_Menu=(key&128)?8:0;
    Input_Clients=cl;Input_ClientCount=1;Input_LastInput=last;W(last,0,std::uint16_t(key));
    const U32 vals[]={a,b,c};
    for(unsigned i=0;i<3;++i){W(cl,0x2c00+4*i,U32(key+i));W(cl,0x2ca4+4*i,vals[i]);W(cl,0x2c18+4*i,float(i+1)*0.25f);}
    W(cl,0x2abc,a);W(cl,0x2c98,b);W(cl,0x2c9c,c);
    std::memset(cmd,0x42,sizeof(cmd));
    Input_Window=0x1234;Input_MouseInitialized=(key>>8)&1;Input_MouseActive=(key>>9)&1;Input_AppActive=(key>>10)&1;
    Input_ShowCursor=42;cursorCount=int((key>>11)&3)-2;
    const Dvar**slots[]={&Input_ClickToContinue,&Input_MouseEnabled};
    for(unsigned i=0;i<2;++i){unsigned k=(key>>(13+i*2))&3;*slots[i]=k?&dvars[i]:nullptr;W(&dvars[i],0x18,int(k==1?1:8));
        if(k==1)W(&dvars[i],0x20,Byte((key>>(17+i))&1));else W(&dvars[i],0x20,(key&(1u<<(17+i)))?"-1":"0");}
    const int client=(key&0x80000)?-1:0;
    if(image){
        W(image,0x25a3308,Input_DemoMode);W(image,0x2653908,Input_PlaybackInited);W(image,0x2653900,Input_Playback);W(image,0xe53a20,Input_Cgame);
        W(image,0xe61794,Input_KeyCatchers);W(image,0x212560c,Input_Menu);
        W(image,0xe61bf8,Input_Clients);W(image,0xe5ca9c,Input_ClientCount);W(image,0x1ef91f4,R<std::uint16_t>(last,0));
        W(image,0x1f02ef8,Input_ClickToContinue);W(image,0x21e2468,Input_MouseEnabled);W(image,0x21e4520,Input_Window);
        W(image,0x420e665,Input_MouseInitialized);W(image,0x420e664,Input_MouseActive);W(image,0x420d61c,Input_AppActive);W(image,0x21e2460,Input_ShowCursor);
        auto*p=static_cast<Byte*>(image);
        switch(op){
        case 0:trace.out[0]=reinterpret_cast<bool(MS*)(int)>(p+0xf1e90)(client);break;
        case 1:reinterpret_cast<void(MS*)(int,Byte*)>(p+0x1a3350)(0,cmd);break;
        case 2:reinterpret_cast<void(MS*)()>(p+0x49a0a0)();Input_MouseActive=R<Byte>(image,0x420e664);Input_ShowCursor=R<int>(image,0x21e2460);break;
        }
    }else switch(op){case 0:trace.out[0]=ShouldUpdateViewAngles(client);break;case 1:FinishMove(0,cmd);break;case 2:InputFrame();break;}
    if(op==1){for(unsigned i=0;i<15;++i)trace.out[i]=R<U32>(cmd,4*i);trace.out[15]=R<U64>(cl,0x2c98);}
    if(op==2){trace.out[0]=Input_MouseActive;trace.out[1]=U32(Input_ShowCursor);trace.out[2]=U32(cursorCount);}
    std::memcpy(out,&trace,sizeof(trace));
}
}
}
