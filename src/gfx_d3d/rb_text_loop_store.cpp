// Store MP DrawTeleType, 140734690 (600 original instructions).
// Recovered from the Store instruction stream, not the older x86 implementation.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_text_loop {
using Byte=unsigned char;using U32=std::uint32_t;using U16=std::uint16_t;
template<class T> static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C"{
extern int Loop_GameTime; // 142c48354
extern U32 Loop_Palette[8],Loop_Color8,Loop_Color9; // 1409b0138,1427f18cc,1427f18c8
const Byte* Loop_Glyph(const Byte*,U32); // 14071a6e0
int Loop_Width(int,const char*,int,const Byte*); // 14071a7a0
void Loop_Quad(const Byte*,float,float,float,float,float,float,float,float,float,float,float,U32); // 14072bfe0
void Loop_Cursor(const Byte*,Byte,float,float,float,float,const Byte*,float,float,U32); // 140732260
float Loop_Legacy(const char*,float,float,float,float,const Byte*,float,float,U32); // 140731e00
float Loop_Named(const char*,float,float,float,float,const Byte*,float,float,U32,int*); // 1407320e0
}
static U32 Decode(const Byte*&p){
    U32 c=*p++;
    if((c&0xe0)==0xc0)c=((c&31)<<6)|(*p++&63);
    else if((c&0xf0)==0xe0){c=((c&15)<<12)|((p[0]&63)<<6)|(p[1]&63);p+=2;}
    return c;
}
// Inlined in both original Store text loops. Inclusive high bound and the
// same-first/different-second branch are deliberately preserved.
static int Kerning(const Byte*font,U32 previous,U32 ch){
    int lo=0,hi=R<int>(font,0x14);
    if(hi<0)return 0;
    const Byte*table=R<const Byte*>(font,0x30);
    while(lo<=hi){
        int mid=(lo+hi)/2;const Byte*p=table+mid*8;
        U32 first=R<U16>(p,0);
        if(first==previous){
            U32 second=R<U16>(p,2);
            if(second==ch)return R<int>(p,4);
            if(second<ch)lo=mid+1;
            hi=mid-1;
        }else if(previous>first)lo=mid+1;
        else hi=mid-1;
    }
    return 0;
}
static U32 ColorCode(U32 color,Byte code){
    if(code=='7')return color;
    U32 rgb=code<'8'?Loop_Palette[code-'0']:code=='8'?Loop_Color8:Loop_Color9;
    return (color&0xff000000u)|(rgb&0xffffffu);
}
static U32 GlyphColor(U32 c,U32 color){
    return (c-1<6||c-14<12||c-28<4||c-0xbc<2)?(color&0xff000000u)|0xffffffu:color;
}
static void DrawGlyph(const Byte*mat,const Byte*g,float x,float y,float z,float sx,float sy,U32 color){
    Loop_Quad(mat,x,y,z,float(g[5])*sx,float(g[6])*sy,R<float>(g,8),R<float>(g,12),R<float>(g,16),R<float>(g,20),0,1,color);
}
static int FollowingWordLength(const Byte*p){
    const Byte*q=p;U32 c=Decode(q);
    while(c<=32&&((0x100002600ull>>c)&1))c=Decode(q);
    const Byte*start=q;
    do{
        if(c<14&&((0x2601u>>c)&1))break;
        c=Decode(q);
    }while(c!=32);
    return int(q-start);
}
void DrawTeleType(const Byte*text,float baseX,float y,float widthOrW,float /*unusedArg5*/,
                  const Byte*font,float sx,float sy,U32 baseColor,U32 flags,int cursorPos,
                  Byte cursorLetter,int /*unusedArg13*/,int birth,int interval){
    float mono=(flags&1)?float(Loop_Glyph(font,'o')[4]):0;
    const Byte*material=R<const Byte*>(font,0x18);
    const int limit=int(U32(Loop_GameTime)-U32(birth))/interval;
    int count=0;U32 previous=0,color=baseColor;
    float x=baseX;
    while(*text&&count<=limit){
        U32 ch=Decode(text);
        if(ch=='^'&&*text!='^'){
            Byte tag=*text;
            if(Byte(tag-'0')<10){color=ColorCode(baseColor,tag);++text;count+=2;continue;}
            if(tag=='F'){++text;count+=2;continue;}
        }
        // UCOMISS equality treats unordered x as equal in this specific branch.
        if((!(x>baseX)&&!(x<baseX)&&ch==' ')||ch==9)continue;
        if(count==limit)ch='_';
        else if(ch=='^'){
            if(Byte(*text-'H')<2){
                x+=Loop_Legacy(reinterpret_cast<const char*>(text),x,y,0,1,font,sx,sy,color);
                text+=int(static_cast<signed char>(text[3]))+4;++count;continue;
            }
            if(*text=='B'){
                int consumed;
                x+=Loop_Named(reinterpret_cast<const char*>(text),x,y,0,1,font,sx,sy,color,&consumed);
                text+=consumed;++count;continue;
            }
        }else if(ch==10){x=baseX;y+=float(R<int>(font,8))*sy;continue;}
        else if(ch==13){x=baseX;continue;}
        if(flags&0x20000){
            float right=baseX+widthOrW;
            bool wrap=!(x<right);
            if(!wrap&&(ch==32||ch==9)){
                int n=FollowingWordLength(text);
                wrap=!(float(Loop_Width(0,reinterpret_cast<const char*>(text),n,font))+x<right);
            }
            if(wrap){x=baseX;y+=float(R<int>(font,8))*sy;continue;}
        }
        const Byte*g=Loop_Glyph(font,ch);
        float kern=float(Kerning(font,previous,ch));previous=ch;
        float ox=float(static_cast<signed char>(g[2]))+kern,oy=float(static_cast<signed char>(g[3]));
        U32 packed=GlyphColor(ch,color);
        if(flags&4){
            float d=flags&8?2.0f:1.0f;
            DrawGlyph(material,g,(ox+d)*sx+x,(d+oy)*sy+y,widthOrW,sx,sy,baseColor&0xff000000u);
        }
        float gx=ox*sx+x;
        DrawGlyph(material,g,gx,oy*sy+y,widthOrW,sx,sy,packed);
        if((flags&2)&&count==cursorPos)Loop_Cursor(material,cursorLetter,gx,y,0,1,font,sx,sy,packed);
        x+=(kern+((flags&1)?mono:float(g[4])))*sx;
        ++count;
    }
    if((flags&2)&&count==cursorPos)Loop_Cursor(material,cursorLetter,x,y,0,1,font,sx,sy,baseColor);
}
}
#endif
