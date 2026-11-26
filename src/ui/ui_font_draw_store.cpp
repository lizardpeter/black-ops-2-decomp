// Faithful Store UI text preprocessing and draw dispatch.
#define T6_SOURCE "ui/ui_main.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font_draw {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T>static T Read(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const Byte*>(p)+o,sizeof(v));return v;}
extern "C" {
extern const Byte *Draw_Language; // 142121178
extern int Draw_TimeInitialized; // 14449E974
extern U32 Draw_TimeOrigin; // 1421E448C
U32 Draw_Tick(); // IAT 1408A7BA0
int Draw_Atoi(const char*);
bool Draw_Replace(int,int*,int*,char*); // 1400F2A90
void Draw_ApplyRect(const Byte*,float*,float*,float*,float*,int,int); // 1401F65E0
void *Draw_Command(const char*,int,const Byte*,float,float,float,float,float,float,const float*,int,int,bool,float); // 14071C880
}
static int Language(const Byte*d){
    if(!d)return 0;
    switch(Read<int>(d,0x18)){
    case 6:case 7:return Read<int>(d,0x20);
    case 1:return Read<Byte>(d,0x20)!=0;
    case 2:return _mm_cvttss_si32(_mm_set_ss(Read<float>(d,0x20)));
    default:return Draw_Atoi(Read<const char*>(d,0x20));
    }
}
// 1403FF700, 68 instructions.
void BlinkText(char*text,U32 size){
    if(Draw_Language&&Language(Draw_Language)==10)return;
    if(!Draw_TimeInitialized){Draw_TimeOrigin=Draw_Tick();Draw_TimeInitialized=1;}
    const int elapsed=static_cast<int>(Draw_Tick()-Draw_TimeOrigin);
    if(elapsed%1000<=800)return;
    for(U32 i=0;i<size&&text[i];++i){
        if(Byte(text[i])==0x10)text[i]=char(0xbc);
        else if(Byte(text[i])==0x11)text[i]=char(0xbd);
    }
}
static float RoundPixel(float x){
    const float v=x+0.5f;const int i=_mm_cvttss_si32(_mm_set_ss(v));
    if(i==(-2147483647-1)||float(i)==v)return v;
    const U32 bits=Read<U32>(&v,0);
    return float(i-int(bits>>31));
}
// 1403FC6D0, 126 instructions. The original requires nonnull text and font.
void DrawTextPadding(int client,const Byte*placement,const char*text,int max,const Byte*font,
                     float x,float y,int ha,int va,float scale,const float*color,int style,float padding){
    char buffer[1024]={};
    if(text){unsigned i=0;do{buffer[i]=text[i];if(!text[i])break;}while(++i<sizeof(buffer));buffer[1023]=0;}
    int index=0,length=0;while(text[length])++length;
    while(Draw_Replace(client,&index,&length,buffer)){}
    BlinkText(buffer,1024);
    float sx=(scale*48.0f)/float(Read<int>(font,8)),sy=sx;
    Draw_ApplyRect(placement,&x,&y,&sx,&sy,ha,va);
    Draw_Command(buffer,max,font,RoundPixel(x),RoundPixel(y),1.0f,sx,sy,0.0f,color,style,-1,false,padding);
}
}
#endif
