// Store MP auxiliary font measurement, text substitution and UI border.
#define T6_SOURCE "gfx_d3d/r_font.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include "gfx_d3d/r_font.h"
#include "ui/ui.h"
namespace store_font_aux {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T> static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C" {
int Aux_IsSpace(int);
void Aux_ResolveText(int,const char*,char*);
extern void* Aux_White;
void Aux_Quad(const float*,float,const float*,void*);
}
template<class Get>static U32 Decode(Get get,int&len){
    U32 c=get(0);len=1;
    if((c&0xe0)==0xc0){len=2;return ((c&31)<<6)|(get(1)&63);}
    if((c&0xf0)==0xe0){len=3;return ((c&15)<<12)|((get(1)&63)<<6)|(get(2)&63);}
    return c;
}
static bool Color(Byte c){return Byte(c-'0')<=9||c=='F';}
static bool LegacyIcon(Byte c){return Byte(c-'H')<=1;}
static int IconWidth(const Byte*font,Byte c){return ((int(static_cast<signed char>(c))-16)*R<int>(font,8)+16)/32;}
// 14071A9C0: byte-limited wrap position, not character count.
const char* WrapPosition(const char*text,int maxBytes,int maxWidth,const Byte*font,float scale){
    const auto*begin=reinterpret_cast<const Byte*>(text);const Byte*p=begin,*space=nullptr,*start=nullptr;
    int width=0;if(maxBytes<=0)maxBytes=0x7fffffff;
    while(*p){
        start=p;int len;U32 ch=Decode([&](int i){return p[i];},len);p+=len;
        if(ch==13){width=0;continue;}
        if(ch==10)return reinterpret_cast<const char*>(p);
        if(ch=='^'&&Color(*p))++p;
        else if(ch=='^'&&LegacyIcon(*p)){
            if(font)width+=IconWidth(font,p[1]);
            p+=int(static_cast<signed char>(p[3]))+4;
            if(start!=begin)space=start;
        }else{
            if(font)width+=R_GetCharacterGlyph(reinterpret_cast<Font_s*>(const_cast<Byte*>(font)),ch)->dx;
            if(start!=begin&&ch<256&&Aux_IsSpace(int(ch)))space=p;
        }
        if(space&&float(width)*scale>float(maxWidth))return reinterpret_cast<const char*>(space);
        if(p-begin>maxBytes)return reinterpret_cast<const char*>(space?space:start);
        if(float(width)*scale>float(maxWidth))return reinterpret_cast<const char*>(start);
    }
    if(p-begin==maxBytes)return reinterpret_cast<const char*>(space?space:start);
    return reinterpret_cast<const char*>(p);
}
// 14071ABE0: measure a span of a power-of-two circular byte buffer.
int RingTextWidth(const Byte*buf,int size,int start,int length,const Byte*font){
    const int mask=size-1,end=(start+length)&mask;int width=0;
    while(start!=end){
        int len;U32 ch=Decode([&](int i){return buf[(start+i)&mask];},len);
        start=(start+len)&mask;
        if(ch=='^'&&Color(buf[start]))start=(start+1)&mask;
        else if(ch=='^'&&LegacyIcon(buf[start])){
            width+=IconWidth(font,buf[(start+1)&mask]);
            // Original payload length read is contiguous; the caller provides the guard.
            start=(start+4+int(static_cast<signed char>(buf[start+3])))&mask;
        }else width+=R_GetCharacterGlyph(reinterpret_cast<Font_s*>(const_cast<Byte*>(font)),ch)->dx;
    }
    return width;
}
// 1400F2A90: one [{...}] substitution. Original caller provides 1024-byte buffer.
bool ReplaceText(int client,int*index,int*length,char*text){
    char copy[1024],key[1024],replacement[1024];
    std::memcpy(copy,text,*length);copy[*length]=0;
    char*open=nullptr,*close=nullptr;
    for(char*p=copy+*index;*p;++p)if(p[0]=='['&&p[1]=='{'){open=p;break;}
    if(!open)return false;
    for(char*p=open;*p;++p)if(p[0]=='}'&&p[1]==']'){close=p;break;}
    if(!close)return false;
    const int n=int(close-open)-2;if(!n)return false;
    std::memcpy(key,open+2,n);key[n]=0;
    Aux_ResolveText(client,key,replacement);
    int count=0;while(replacement[count])++count;
    const int total=*length-n-4+count;if(total+1>1024)return false;
    const int at=int(open-copy);
    std::memcpy(text+at,replacement,count);
    const int tail=total-at-count;
    std::memcpy(text+at+count,close+2,tail);text[total]=0;
    *index=at+count;*length=total;return true;
}
// 1403FC430: beveled border with two colors, original two placement calls.
void DrawBorder(const Byte*placement,float x,float y,float w,float h,int ha,int va,
                float thickness,const float*light,const float*dark){
    ScrPlace_ApplyRect(reinterpret_cast<const ScreenPlacement*>(placement),&x,&y,&w,&h,ha,va);
    float zeroX=0,zeroY=0,tX=thickness,tY=thickness;
    ScrPlace_ApplyRect(reinterpret_cast<const ScreenPlacement*>(placement),&zeroX,&zeroY,&tX,&tY,ha,va);
    const float right=x+w,bottom=y+h,innerRight=right-tX,innerLeft=x+tX,innerTop=y+tY,innerBottom=bottom-tY;
    float a[8]={x,y,right,y,innerRight,innerTop,innerLeft,innerTop};Aux_Quad(a,1,light,Aux_White);
    float b[8]={x,y,innerLeft,innerTop,innerLeft,innerBottom,x,bottom};Aux_Quad(b,1,light,Aux_White);
    float c[8]={x,bottom,innerLeft,innerBottom,innerRight,innerBottom,right,bottom};Aux_Quad(c,1,dark,Aux_White);
    float d[8]={right,y,right,bottom,innerRight,innerBottom,innerRight,innerTop};Aux_Quad(d,1,dark,Aux_White);
}
}
#endif
