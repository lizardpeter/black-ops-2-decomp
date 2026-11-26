// Store MP original material registration, translation and edit-field text draw.
#define T6_SOURCE "ui/ui_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstring>
#include <cstdint>
namespace store_font_core{void*DefaultFont(float);}
namespace store_font{void ApplyRect(const unsigned char*,float*,float*,float*,float*,int,int);}
namespace store_text_cmd{unsigned char* AddBaseDrawTextCmd(const char*,int,const void*,float,float,float,float,float,float,const float*,int,int,unsigned char,float);}
namespace store_font_ui_support {
using Byte=unsigned char;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
extern "C"{
void*Support_FindAsset(int,const char*,bool,int);
const char*Support_Translate(const char*);
void Support_MissingTranslation(const char*);
void Support_TranslationOverflow();
int Support_Atoi(const char*);
extern const Byte*Support_WarnMissing,*Support_WarnAsError;
extern char Support_TranslationBuffer[1024];
extern const Byte*Support_FieldFont,*Support_FieldPlacement,*Support_Clients;
extern float Support_HudScale,Support_CaptureY;
extern int Support_Capture;
extern const float*Support_White;
}
static bool Bool(const Byte*d){return d&&(R<int>(d,0x18)==1?d[0x20]!=0:Support_Atoi(R<const char*>(d,0x20))!=0);}
// 14071A4E0: original fastfile material registration tail-call.
void*RegisterMaterial(const char*name,int,bool allowDefault,int zone){return Support_FindAsset(6,name,allowDefault,zone);}
static void CopyBounded(char*d,const char*s,int n){
    if(!d)return;
    if(!s){*d=0;return;}
    for(int i=0;i<n;++i){d[i]=s[i];if(!s[i])break;}
    d[n-1]=0;
}
static int Length(const char*p){int n=0;while(p[n])++n;return n;}
static void Append(char*d,const char*s){
    int n=Length(d);if(n>1023)Support_TranslationOverflow();
    CopyBounded(d+n,s,1024-n);
}
// 1403FF450: safe translation with optional colored missing-localization markup.
const char*SafeTranslate(const char*key){
    if(*key==0x15)++key;
    else{
        if(!key[0]||!key[1]||key[0]=='^')return key;
        if(const char*s=Support_Translate(key))return s;
    }
    if(Bool(Support_WarnMissing)){
        if(Bool(Support_WarnAsError))Support_MissingTranslation(key);
        CopyBounded(Support_TranslationBuffer,"^1UNLOCALIZED(^7",1024);
        Append(Support_TranslationBuffer,key);Append(Support_TranslationBuffer,"^1)^7");
    }else{
        int n=0;do{Support_TranslationBuffer[n]=key[n];}while(key[n++]);
    }
    return Support_TranslationBuffer;
}
// 1401A43A0: original field visible segment and cursor.
void DrawTextField(int client,Byte*field,int x,int y){
    int max=R<int>(field,8);if(!max){max=256;W(field,8,max);}
    int scroll=R<int>(field,4),count=256-scroll;
    char visible[1024];
    if(!count)visible[0]=0;
    else CopyBounded(visible,reinterpret_cast<const char*>(field+0x18+scroll),count);
    int cursor=R<int>(field,0)-scroll;
    const Byte*font;float scale;int style;
    const bool cursorMode=R<int>(Support_Clients,client*0x1128+0x11c)!=0;
    const Byte letter=cursorMode?0x5f:0x7c;
    if(R<int>(field,0x14)){
        font=Support_FieldFont;scale=1;style=0;
    }else{
        scale=R<float>(field,0x10);
        float choose=((Support_Capture?Support_CaptureY:1.0f)*(((scale*(1.0f/48.0f))*Support_HudScale)*Support_HudScale))*R<float>(Support_FieldPlacement,4);
        font=static_cast<const Byte*>(store_font_core::DefaultFont(choose));scale/=float(R<int>(font,8));style=3;
    }
    float px=float(x),py=float(R<int>(font,8))*scale+float(y),sx=scale,sy=scale;
    store_font::ApplyRect(Support_FieldPlacement,&px,&py,&sx,&sy,1,1);
    store_text_cmd::AddBaseDrawTextCmd(visible,max,font,px,py,1,sx,sy,0,Support_White,style,cursor,letter,0);
}
}
#endif
