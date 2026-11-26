// Original Store MP font lookup and selection bodies. No enhanced behavior.
#define T6_SOURCE "gfx_d3d/r_font.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font_core {
using Byte=unsigned char;
using U32=std::uint32_t;
template<class T> static T Read(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const Byte*>(p)+off,sizeof(v));return v;}
extern "C" {
extern const Byte *Core_Small,*Core_Big,*Core_ExtraBig;
extern void *Core_Assets[21];
extern int Core_Capture;
extern float Core_CaptureY,Core_HudScale;
double Core_Atof(const char*);
void *Core_FindAsset(int,const char*,bool,int);
void Core_Icon(int,const char*,int,void**,float*,float*);
}
static float DvarFloat(const Byte*d){
    const int type=Read<int>(d,0x18);
    if(type==2)return Read<float>(d,0x20);
    if(type==6)return float(Read<int>(d,0x20));
    return float(Core_Atof(Read<const char*>(d,0x20)));
}
// COMISS/JBE takes the unordered branch as well.
static bool BelowOrEqualOrUnordered(float a,float b){return !(a>b);}
void *DefaultFont(float scale){
    if(Core_Small&&BelowOrEqualOrUnordered(scale,DvarFloat(Core_Small)))return Core_Assets[0x70/8];
    if(Core_ExtraBig&&scale>=DvarFloat(Core_ExtraBig))return Core_Assets[0x90/8];
    if(Core_Big&&scale>=DvarFloat(Core_Big))return Core_Assets[0x68/8];
    return Core_Assets[0x88/8];
}
void *FontHandle(const Byte*placement,int kind,float scale){
    switch(kind){
    case 1:return Core_Assets[0x88/8];case 2:return Core_Assets[0x68/8];
    case 3:return Core_Assets[0x70/8];case 5:return Core_Assets[0x78/8];
    case 6:return Core_Assets[0x90/8];case 7:return Core_Assets[0x98/8];case 8:return Core_Assets[0xa0/8];
    default:break;
    }
    scale=((Core_Capture?Core_CaptureY:1.0f)*((scale*Core_HudScale)*Core_HudScale))*Read<float>(placement,4);
    if(kind!=4)return DefaultFont(scale);
    if(Core_Small&&BelowOrEqualOrUnordered(scale,DvarFloat(Core_Small)))return Core_Assets[0x70/8];
    if(Core_Big&&scale>=DvarFloat(Core_Big))return Core_Assets[0x80/8];
    return Core_Assets[0x88/8];
}
// Font_s: glyphCount +0x10, glyphs +0x28; Glyph stride 24.
const Byte *CharacterGlyph(const Byte*font,U32 letter){
    const Byte*glyphs=Read<const Byte*>(font,0x28);
    if(letter-0x20u<=0x5fu)return glyphs+(letter-0x20u)*24;
    int low=96,high=Read<int>(font,0x10)-1;
    while(low<=high){
        const int mid=(low+high)/2;const Byte*g=glyphs+mid*24;
        const U32 code=Read<std::uint16_t>(g,0);
        if(code==letter)return g;
        if(code<letter)low=mid+1;else high=mid-1;
    }
    return glyphs+14*24;
}
void *RegisterFont(const char*name){return Core_FindAsset(20,name,true,-1);}
int TextWidth(int client,const char*text,int maxChars,const Byte*font){
    const auto*p=reinterpret_cast<const Byte*>(text);
    int current=0,best=0,count=0;
    if(maxChars<=0)maxChars=0x7fffffff;
    while(*p&&count<maxChars){
        U32 ch=*p;
        if((ch&0xe0)==0xc0){ch=((ch&31)<<6)|(p[1]&63);p+=2;}
        else if((ch&0xf0)==0xe0){ch=((ch&15)<<12)|((p[1]&63)<<6)|(p[2]&63);p+=3;}
        else ++p;
        if(ch==20||ch==21)continue;
        if(ch==10||ch==13){current=0;continue;}
        if(ch=='^'){
            const Byte tag=*p;
            if((tag>='0'&&tag<='9')||tag=='F'||tag=='L'){++p;continue;}
            if(tag=='C'&&p[1]>='0'&&p[1]<='9'){p+=2;continue;}
            if(tag=='B'){
                char name[64];unsigned n=0;
                while(n<63&&p[n+1]!='^'){name[n]=char(p[n+1]);++n;}
                name[n]=0;
                const int height=Read<int>(font,8);void*material;float width,heightScale;
                Core_Icon(client,name,height,&material,&width,&heightScale);
                current+=_mm_cvttss_si32(_mm_set_ss(float(height)*width));
                p+=n+2;
                if(current>best)best=current;
                continue;
            }
        }
        ++count;current+=CharacterGlyph(font,ch)[4];
        if(current>best)best=current;
    }
    return best;
}
}
#endif
