// Original Store MP UI text call sites, with original argument positions.
#define T6_SOURCE "ui/ui_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font{void ApplyRect(const unsigned char*,float*,float*,float*,float*,int,int);}
#include "ui/ui.h"
namespace store_text_cmd{
unsigned char* AddBaseDrawTextCmd(const char*,int,const void*,float,float,float,float,float,float,const float*,int,int,unsigned char,float);
void AddCmdDrawTextEffects(const char*,int,const void*,float,float,float,float,float,float,const float*,int,const float*,const void*,const void*,int,int,int,int);
}
namespace store_ui_wrappers{
using B=unsigned char;using U=std::uint32_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
extern "C"{
extern const B* Wrap_Language,*Wrap_Placements,*Wrap_ClientTable,*Wrap_ClockState,*Wrap_ClockDvar,*Wrap_DistanceDvar;
extern int Wrap_Capture;extern float Wrap_CaptureY,Wrap_HudScale;
extern const void* Wrap_FxMaterial,*Wrap_FxGlow;
int Wrap_Atoi(const char*);
double Wrap_Atof(const char*);
void Wrap_PackColor(const float*,B*);
int Wrap_TextWidth(int,const char*,int,const B*);
void Wrap_FormatInt(char*,unsigned,const char*,int);
void Wrap_FormatDouble(char*,unsigned,const char*,double);
}
using store_font::ApplyRect;
using store_text_cmd::AddBaseDrawTextCmd;
static int DvarInt(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(unsigned(t-6)<2)return R<int>(d,0x20);
    if(t==1)return d[0x20]!=0;
    if(t==2)return _mm_cvttss_si32(_mm_set_ss(R<float>(d,0x20)));
    return Wrap_Atoi(R<const char*>(d,0x20));
}
static float DvarFloat(const B*d){
    if(!d)return 0;
    int t=R<int>(d,0x18);
    if(t==2)return R<float>(d,0x20);
    if(t==6)return float(R<int>(d,0x20));
    return float(Wrap_Atof(R<const char*>(d,0x20)));
}
static float RoundPixel(float v){
    v+=0.5f;int n=_mm_cvttss_si32(_mm_set_ss(v));
    if(n==(-2147483647-1)||float(n)==v)return v;
    return float(n-int(R<U>(&v,0)>>31));
}
static const B* Placement(int c){
    const B*p=Wrap_ClientTable+static_cast<long long>(c)*24;
    int index=R<int>(p,0)==c?R<int>(p,8):-1;
    return Wrap_Placements+static_cast<long long>(index)*0x7c;
}
static void Glow(B*p,const float*color,const float*g){
    if(g[3]>0||g[3]<0){
        W(p,0x38,R<U>(p,0x38)|0x30u);
        float v[4]={g[0]*0.1f,g[1]*0.1f,g[2]*0.1f,color[3]*g[3]};
        Wrap_PackColor(v,p+0x44);
    }
}
// 1401B3050
void DrawTextScaled(const B*p,const char*t,int n,const B*f,float x,float y,int ha,int va,float sx,float sy,const float*c,int style){
    ApplyRect(p,&x,&y,&sx,&sy,ha,va);
    AddBaseDrawTextCmd(t,n,f,x,y,1,sx,sy,0,c,style,-1,0,0);
}
// 1401B3130
void DrawTextRotated(const B*p,const char*t,int n,const B*f,float x,float y,float angle,int ha,int va,float sx,float sy,const float*c,int style){
    ApplyRect(p,&x,&y,&sx,&sy,ha,va);
    AddBaseDrawTextCmd(t,n,f,x,y,1,sx,sy,angle,c,style,-1,0,0);
}
// 1403FCC10
void DrawTextNormalized(const B*p,const char*t,int /*unused*/,const B*f,float x,float y,int ha,int va,float scale,const float*c,int style){
    float sx=(scale*48.0f)/float(R<int>(f,8)),sy=sx;
    ApplyRect(p,&x,&y,&sx,&sy,ha,va);
    AddBaseDrawTextCmd(t,0x7fffffff,f,x,y,1,sx,sy,0,c,style,-1,0,0);
}
// 140426EB0
void* ItemFont(int client,const B*item){
    const B*p=Wrap_Placements+static_cast<long long>(client)*0x7c;
    if(const B*font=R<const B*>(item,0xe8))return UI_GetFontHandle(reinterpret_cast<const ScreenPlacement*>(p),R<int>(font,0x1c),R<float>(font,0x30));
    return UI_GetFontHandleDefault(((Wrap_Capture?Wrap_CaptureY:1.0f)*((Wrap_HudScale*0.55f)*Wrap_HudScale))*R<float>(p,4));
}
// 1403A5D90. Only AL is defined by this original ABI.
B DecimalSeparator(){
    U language=U(DvarInt(Wrap_Language));
    return Wrap_Language&&language<=12&&((0x136eu>>language)&1)?',':'.';
}
// 1403FC930
void DrawTextGlow(const B*p,const char*t,int /*unused*/,const B*f,float x,float y,int ha,int va,float scale,const float*c,int style,const float*g,B special,B variant){
    float sx=(scale*48.0f)/float(R<int>(f,8)),sy=sx;
    ApplyRect(p,&x,&y,&sx,&sy,ha,va);x=RoundPixel(x);y=RoundPixel(y);
    if(!special){
        store_text_cmd::AddCmdDrawTextEffects(t,0x7fffffff,f,x,y,1,sx,sy,0,c,style,g,nullptr,nullptr,0,0,0,0);
        return;
    }
    if(B*out=AddBaseDrawTextCmd(t,0x7fffffff,f,x,y,1,sx,sy,0,c,style,-1,0,0)){
        W(out,0x38,R<U>(out,0x38)|0x100u|(variant?0x200u:0u));Glow(out,c,g);
    }
}
// 1400C9C80
void DrawHudText(int client,const B*position,const B*font,float scale,const float*color,float /*unused*/,int style,const B*effect,const char*text){
    const B*p=Placement(client);
    if(!text||!(color[3]>0||color[3]<0))return;
    float sx=(scale*48.0f)/float(R<int>(font,8)),sy=sx;
    U letter=U(DvarInt(Wrap_ClockDvar));int birth=R<int>(Wrap_ClockState,0x84930);
    const void*m=Wrap_FxMaterial,*g=Wrap_FxGlow;
    float x=R<float>(position,0),y=R<float>(position,4);
    ApplyRect(p,&x,&y,&sx,&sy,R<int>(position,16),R<int>(position,20));
    if(B*out=AddBaseDrawTextCmd(text,64,font,x,y,1,sx,sy,0,color,style,-1,0,0)){
        float glow[4];std::memcpy(glow,effect+0x94,16);Glow(out,color,glow);
        if(birth){W(out,0x38,R<U>(out,0x38)|0x2000u);W(out,0x60,m);W(out,0x68,g);W(out,0x48,birth);W(out,0x4c,letter);W(out,0x50,4000);W(out,0x54,1000);W(out,0x70,0.0f);}
    }
}
// 1400CADB0
void DrawDistance(int client,float distance,float center,float y,float offset,const B*pos,const float*color,const B*font,float scale,int style){
    float c[4]={0.9f,1.0f,0.1f,color[3]};char label[160];
    float threshold=DvarFloat(Wrap_DistanceDvar);
    if(!(threshold>distance))Wrap_FormatInt(label,64,"%im",_mm_cvttss_si32(_mm_set_ss(distance)));
    else Wrap_FormatDouble(label,64,"%.1fm",double(float(_mm_cvttss_si32(_mm_set_ss(distance*10.0f)))*0.1f));
    float sx=(scale*48.0f)/float(R<int>(font,8)),sy=sx;
    int width=Wrap_TextWidth(client,label,20,font);
    float x=center-float(_mm_cvttss_si32(_mm_set_ss(RoundPixel(float(width)*sx))))*0.5f;
    y=y+offset+2.0f;
    ApplyRect(Placement(client),&x,&y,&sx,&sy,R<int>(pos,16),R<int>(pos,20));
    AddBaseDrawTextCmd(label,0x7fffffff,font,x,y,1,sx,sy,0,c,style,-1,0,0);
}
}
#endif
