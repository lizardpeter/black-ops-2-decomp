// Original Store MP HUD client selection and chat dispatch.
#define T6_SOURCE "cgame/cg_hud.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_loc_format{const char*TranslateExe(const char*);}
namespace store_font{void ApplyRect(const unsigned char*,float*,float*,float*,float*,int,int);}
#include "ui/ui.h"
namespace store_text_cmd{unsigned char* AddBaseDrawTextCmd(const char*,int,const void*,float,float,float,float,float,float,const float*,int,int,unsigned char,float);}
namespace store_hud_clients{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const B*>(p)+n,sizeof v);return v;}
extern "C"{
extern const B *Clients_Count,*Clients_Enable,*Clients_State,*Clients_Color[6],*Clients_Relation,*Clients_Placement;
extern B* Clients_Chat;
extern U Clients_Mode,Clients_Flags,Clients_ChatFlags;
extern int Clients_Capture;extern float Clients_CaptureY,Clients_HudScale;
int Clients_Atoi(const char*);
void Clients_ParseColor(const char*,U*);
const char* Clients_Format(const char*,const char*);
int Clients_Width(int,const char*,int,const void*);
void Clients_Edit(int,B*,int,int);
}
static int Int(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(U(t-6)<2)return R<int>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Clients_Atoi(R<const char*>(d,0x20));
}
// 1400A9960: only AL is meaningful in the original return value.
bool HudEnabled(){return Int(Clients_Enable)!=0;}
// 1400EA250
int FindHudClient(const void* /*unused*/,const B*entity){
    int n=Int(Clients_Count);
    for(int i=0;i<n;++i){
        const B*p=Clients_State+0x6bfa0+static_cast<long long>(i)*0x870;
        if(R<int>(p-0x618,0)&&R<int>(p-4,0)==R<int>(entity,500)&&!R<int>(p,0))return i;
    }
    return 1023;
}
// 1400DE7B0: RGB only, alpha is preserved.
void ClientColor(int target,float*out){
    int self=R<int>(Clients_State,0),view=self;
    const B*s=R<const B*>(Clients_State,0x30);
    auto team=[](int i){return R<int>(Clients_State+static_cast<long long>(i)*0x870,0x6b9b4);};
    if(s&&(team(self)==9||(Clients_Mode==2&&(Clients_Flags&4))))view=R<int>(s,0x1b0);
    int t=team(target);unsigned index;
    if(t==9)index=0;
    else if(target==view||(t&&team(view)==t)){
        if(target!=self&&Clients_Mode!=2&&Clients_Relation[target])index=3;
        else index=Clients_State[0x6a3b4]?2:1;
    }else index=Clients_State[0x6a3b4]?5:4;
    const B*d=Clients_Color[index];
    U color;
    if(R<int>(d,0x18)==9)color=R<U>(d,0x20);
    else Clients_ParseColor(R<const char*>(d,0x20),&color);
    for(unsigned i=0;i<3;++i)out[i]=float((color>>(i*8))&255)*0.003921569f;
}
// 1400D2FF0: original chat prompt followed by editable field.
void DrawChatPrompt(int client){
    if(client==-1||!(Clients_ChatFlags&32))return;
    B*chat=Clients_Chat+static_cast<long long>(client)*0x1128;
    const char*label=store_loc_format::TranslateExe(R<int>(chat,0x118)?"EXE_SAYTEAM":"EXE_SAY");
    label=Clients_Format("%s: ",label);
    float size=R<float>(chat,0x10);
    float scale=(Clients_Capture?Clients_CaptureY:1.0f)*(((size*0.020833334f)*Clients_HudScale)*Clients_HudScale)*R<float>(Clients_Placement,4);
    const void*font=UI_GetFontHandleDefault(scale);
    float height=float(R<int>(font,8));scale=size/height;
    float x=5,y=float(_mm_cvttss_si32(_mm_set_ss(height*scale))+254),sx=scale,sy=scale;
    store_font::ApplyRect(Clients_Placement,&x,&y,&sx,&sy,1,1);
    const float white[]={1,1,1,1};
    store_text_cmd::AddBaseDrawTextCmd(label,0x7fffffff,font,x,y,1,sx,sy,0,white,3,-1,0,0);
    Clients_Edit(client,chat,_mm_cvttss_si32(_mm_set_ss(float(Clients_Width(0,label,0,font))*scale))+5,254);
}
}
#endif
