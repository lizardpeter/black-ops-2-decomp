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
// Store 140732480: the original 24-argument 2D backend.
// Helpers below correspond to regions in this one body, not separate functions.
extern "C" {
extern int Loop_TimerInitialized,Loop_BlinkBase,Loop_BlinkElapsed;
extern U32 Loop_TimerOrigin;
extern Byte Loop_BlinkVisible;
extern Byte Loop_JitterPattern[176];
extern float Loop_GlowOffsets[8];
extern const Byte*Loop_RedactionMaterials[3];
U32 Loop_Tick();
int Loop_PrintLength(const Byte*);
void Loop_Stretch(const Byte*,float,float,float,float,float,float,float,float,U32);
void Loop_Slice(const Byte*,int,float,float,float,float,float,float,float,U32);
}
struct TextFx{
    int limit,decay,decayBucket,obscured;U32 seed;
    bool expired,fade,randomLast,underscoreLast,largeLast;
    float lastScale;
};
static U32 Next(U32&seed){seed=seed*0x41c64e6du+0x3039;return seed;}
static int Random15(U32 seed){return (int(seed+(int(seed)<0?65535u:0u))>>16)&32767;}
static int Rand(U32&seed){return Random15(Next(seed));}
static int Elapsed(int birth){return int(U32(Loop_GameTime)-U32(birth));}
static U32 Seed(int value){return 0xd3dc167eu-U32(value)*0x3d5d6597u;}
static int Min(int a,int b){return a<b?a:b;}
static TextFx SetupFx(const Byte*text,int max,U32 flags,int birth,int letter,int fadeStart,int duration){
    TextFx fx{max,0,0,0,1,false,false,false,false,false,1.0f};
    int elapsed=Elapsed(birth);
    if(flags&0x800){
        if(elapsed>fadeStart+duration){fx.expired=true;return fx;}
        fx.seed=0;
        if(elapsed>birth){fx.fade=elapsed>fadeStart;fx.decay=fx.fade?elapsed-fadeStart:0;}
        else{fx.fade=true;fx.decay=duration-elapsed;}
        fx.decayBucket=fx.decay;
    }else if(flags&0x400||flags&0x1000||flags&0x2000){
        fx.seed=0;
        fx.underscoreLast=!(flags&0x400)&&(flags&0x1000);
        fx.largeLast=!(flags&0x400)&&!(flags&0x1000)&&(flags&0x2000);
        if(fx.largeLast)fx.lastScale=2.0f;
        int n=Min(max,Loop_PrintLength(text));
        if(elapsed>fadeStart+duration){fx.expired=true;return fx;}
        if(elapsed>n*letter){fx.fade=elapsed>fadeStart;fx.decay=fx.fade?elapsed-fadeStart:0;}
        else fx.limit=elapsed/letter;
        fx.decayBucket=fx.decay;
    }else if(flags&0x4000){
        if(elapsed>fadeStart+duration){fx.expired=true;return fx;}
        fx.fade=elapsed>fadeStart;fx.decay=fx.fade?elapsed-fadeStart:0;
        fx.decayBucket=fx.decay;
    }else if(flags&0x10000){
        int n=Min(max,Loop_PrintLength(text));
        if(elapsed>fadeStart+duration){fx.expired=true;return fx;}
        if(elapsed>n*letter&&elapsed>fadeStart){
            fx.fade=true;fx.decay=elapsed-fadeStart;fx.decayBucket=fx.decay;
        }else{
            fx.limit=n;fx.obscured=n-elapsed/letter;fx.seed=Seed(Loop_GameTime/50);
        }
    }else if(flags&0x40){
        int n=Min(max,Loop_PrintLength(text));
        if(elapsed>fadeStart+duration){fx.expired=true;return fx;}
        if(elapsed<n*letter){
            int whole=elapsed/letter;int rem=elapsed-whole*letter;
            if(letter/4)rem/=letter/4;
            fx.randomLast=true;fx.limit=whole+1;fx.seed=Seed(rem+n+whole+birth);
        }else if(elapsed>fadeStart){
            fx.fade=true;fx.decay=elapsed-fadeStart;fx.decayBucket=fx.decay;fx.seed=Seed(n+birth);
        }
    }
    return fx;
}
static void UpdateBlink(){
    auto now=[](){
        if(!Loop_TimerInitialized){Loop_TimerOrigin=Loop_Tick();Loop_TimerInitialized=1;}
        return int(Loop_Tick()-Loop_TimerOrigin);
    };
    if(!Loop_BlinkBase)Loop_BlinkBase=now();
    Loop_BlinkElapsed=int(U32(now())-U32(Loop_BlinkBase));
    if(Loop_BlinkElapsed>175){Loop_BlinkBase=now();Loop_BlinkVisible=Byte(!Loop_BlinkVisible);}
}
static void Jitter(float&x,float&y){
    U32 seed=U32(Loop_GameTime>>5);
    Byte kind=Loop_JitterPattern[static_cast<std::uint64_t>(static_cast<long long>(int(seed)))%176u];
    if(kind==1){x+=float(Rand(seed)%7-3);y+=float(Rand(seed)%11-5);}
    else if(kind==2){x+=float(Rand(seed)%10+45);int delta=Rand(seed)%10+45;y+=float((Rand(seed)&1)?-delta:delta);}
}
static void Rotate(float x,float y,float baseX,float baseY,float sn,float cs,float&rx,float&ry){
    float dx=x-baseX,dy=y-baseY;
    rx=(dx*cs+baseX)-dy*sn;ry=(dy*cs+baseY)+dx*sn;
}
static U32 WithAlpha(U32 color,int alpha){return (color&0xffffffu)|(U32(Byte(alpha))<<24);}
static Byte FadeByte(int decay,int duration,U32 color){
    if(int(float(duration)*0.030000001f)<1)return 0;
    float a=1;
    if(decay<=duration)a=(1.0f-float(decay)/float(duration))*float(color>>24)*0.003921569f;
    float f=a*255.0f+0.5f;int n=int(f);
    if(float(n)!=f&&f<0.0f)--n;
    return Byte(n<1?0:n>255?255:n);
}
static U32 RawPalette(Byte code){
    if(Byte(code-'0')>=10)code='7';
    return code<'8'?Loop_Palette[code-'0']:code=='8'?Loop_Color8:Loop_Color9;
}
static U32 InterpolateRgb(U32 a,U32 b,float fraction){
    U32 result=a&0xff000000u;
    for(unsigned shift=0;shift<24;shift+=8){
        int av=Byte(a>>shift),bv=Byte(b>>shift);
        result|=U32(Byte(int(float(bv-av)*fraction+float(av))))<<shift;
    }
    return result;
}
struct Bounds{float left,top,right,bottom;};
static void OpenBounds(Bounds&b){b={3.402823466e38f,3.402823466e38f,-3.402823466e38f,-3.402823466e38f};}
static void GlyphQuad(const Byte*m,const Byte*g,float x,float y,float w,float sx,float sy,float sn,float cs,U32 color,float scaleW=1,float scaleH=1){
    Loop_Quad(m,x,y,w,(float(g[5])*sx)*scaleW,(float(g[6])*sy)*scaleH,
              R<float>(g,8),R<float>(g,12),R<float>(g,16),R<float>(g,20),sn,cs,color);
}
void DrawText2D(const Byte*text,float baseX,float baseY,float w,const Byte*font,
                float sx,float sy,float sn,float cs,U32 baseColor,int max,U32 flags,
                int cursor,Byte cursorLetter,float padding,U32 glowColor,int birth,int letter,
                int fadeStart,int duration,int redactStart,int redactDuration,
                const Byte*fxMaterial,const Byte*fxGlow){
    const float mono=(flags&1)?float(Loop_Glyph(font,'o')[4]):0;
    const Byte*material=R<const Byte*>(font,0x18);
    UpdateBlink();
    TextFx fx=SetupFx(text,max,flags,birth,letter,fadeStart,duration);
    if(fx.expired)return;
    const Byte*glow=(flags&0x10)?R<const Byte*>(font,0x20):nullptr;
    int passes=(flags&0x10)?2:1;
    if(flags&0x8000)Jitter(baseX,baseY);
    bool blinking=false,redact=false,underline=false,shiftColor=false;
    Byte shiftCode=0;
    Bounds red[4]{},under[4]{};
    int redCount=0,underCount=0;
    float minY=3.402823466e38f,maxY=-3.402823466e38f,minX=3.402823466e38f,maxX=-3.402823466e38f;
    int lastAlpha=255;
    U32 shadow=baseColor&0xff000000u;
    for(int pass=0;pass<passes;++pass){
        const Byte*p=text;
        int remaining=fx.limit,count=0;
        U32 previous=0,color=baseColor,seed=fx.seed;
        bool specialColor=false;
        float x=baseX,y=baseY;
        while(*p&&remaining){
            U32 ch=Decode(p);
            if(ch=='^'&&*p!='^'){
                Byte tag=*p;
                if(Byte(tag-'0')<10){
                    specialColor=false;
                    if((flags&0x100)&&tag=='2'){
                        specialColor=true;
                        color=0xe6ffe6u|(U32(Byte(int(float(baseColor>>24)*220.0f*0.003921569f)))<<24);
                    }else color=ColorCode(baseColor,tag);
                    ++p;count+=2;continue;
                }
                if(tag=='F'){blinking=!blinking;++p;count+=2;continue;}
            }
            if((flags&0x4000)&&ch=='^'){
                if(*p=='C'){
                    ++p;
                    if(shiftColor){shiftColor=false;++count;}
                    else{shiftColor=true;shiftCode=*p++;count+=3;}
                }else if(*p=='L'){
                    if(redact){redact=false;++redCount;++p;count+=2;}
                    else if(redCount<4){redact=true;OpenBounds(red[redCount]);++p;count+=2;}
                }else if(*p=='U'){
                    if(underline){underline=false;++underCount;++p;count+=2;}
                    else if(underCount<4){underline=true;OpenBounds(under[underCount]);++p;count+=2;}
                }
                continue;
            }
            bool extra=false,hide=false,randomAtlas=false;
            int atlas=0,alphaFactor=0;
            bool skipControl=false;
            if(remaining<=fx.obscured&&ch!=32){
                const int n=Rand(seed);
                U32 tmp=U32(n);ch=Byte("023456789"[Rand(tmp)%9]);alphaFactor=192;skipControl=true;
            }else if(fx.obscured>0){alphaFactor=255;skipControl=true;}
            else if(fx.randomLast&&remaining==1){
                int n=Rand(seed);ch=Byte("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890"[n%62]);
                if(n&1){ch='O';extra=true;randomAtlas=true;}
                alphaFactor=192;skipControl=true;
            }
            if(fx.underscoreLast&&remaining==1){ch='_';skipControl=true;}
            if(!skipControl){
                if(ch==10){x=baseX;y+=float(R<int>(font,8))*sy;continue;}
                if(ch==13){x=baseX;continue;}
                if(ch=='^'&&Byte(*p-'H')<2){
                    float advance;
                    if(blinking&&!Loop_BlinkVisible)advance=float(((int(static_cast<signed char>(p[1]))-16)*R<int>(font,8)+16)/32)*sx;
                    else{
                        float rx,ry;Rotate(x,y,baseX,baseY,sn,cs,rx,ry);
                        advance=Loop_Legacy(reinterpret_cast<const char*>(p),rx,ry,sn,cs,font,sx,sy,color);
                    }
                    x+=advance;
                    if(flags&0x80)x+=sx*padding;
                    p+=int(static_cast<signed char>(p[3]))+4;++count;--remaining;continue;
                }
                if(ch=='^'&&*p=='B'){
                    float rx,ry;Rotate(x,y,baseX,baseY,sn,cs,rx,ry);int consumed;
                    x+=Loop_Named(reinterpret_cast<const char*>(p),rx,ry,sn,cs,font,sx,sy,blinking&&!Loop_BlinkVisible?0xffffffu:baseColor,&consumed);
                    if(flags&0x80)x+=sx*padding;
                    p+=consumed;++count;--remaining;continue;
                }
            }
            if(fx.fade){
                if(flags&0x1000){
                    alphaFactor=255;extra=false;
                    int n=int(float(duration)*0.030000001f);
                    hide=(Rand(seed)%n)*(duration/n)<=fx.decayBucket;
                    randomAtlas=false;
                }else alphaFactor=FadeByte(fx.decay,duration,color);
            }
            if(randomAtlas)atlas=Random15(seed*0x41c64e6du+0x3039);
            const Byte*g=Loop_Glyph(font,ch);
            x+=float(Kerning(font,previous,ch))*sx;previous=ch;
            float ox=float(static_cast<signed char>(g[2]))*sx,oy=float(static_cast<signed char>(g[3]))*sy;
            if(fx.largeLast&&remaining==1){sx*=fx.lastScale;sy*=fx.lastScale;oy=(fx.lastScale+1.0f)*oy*0.5f;}
            U32 packed=GlyphColor(ch,color);
            if(fx.fade||(fx.randomLast&&remaining==1)||fx.obscured>0){
                shadow=WithAlpha(shadow,alphaFactor);
                packed=WithAlpha(packed,int(float(packed>>24)*float(alphaFactor)*0.003921569f));
            }
            int glyphAlpha=int(packed>>24);
            if(shiftColor){
                U32 target=WithAlpha(RawPalette(shiftCode),glyphAlpha);
                int elapsed=Elapsed(birth);
                if(elapsed>redactStart+redactDuration*2)packed=target;
                else if(elapsed>redactStart+redactDuration)packed=InterpolateRgb(packed,target,float(elapsed-redactStart-redactDuration)/float(redactDuration));
            }
            if(!hide){
                const bool visible=!blinking||Loop_BlinkVisible;
                if(pass==0){
                    if((flags&4)&&visible){
                        float d=flags&8?2.0f:1.0f,rx,ry;
                        Rotate((ox+x)+d*sx,(oy+y)+d*sy,baseX,baseY,sn,cs,rx,ry);
                        if(extra)Loop_Slice(fxMaterial,atlas,rx,ry,w,float(g[5])*sx,float(g[6])*sy,sn,cs,shadow);
                        else GlyphQuad(material,g,rx,ry,w,sx,sy,sn,cs,shadow);
                    }
                    float rx,ry;Rotate(ox+x,oy+y,baseX,baseY,sn,cs,rx,ry);
                    if(visible){
                        if(extra)Loop_Slice(fxMaterial,atlas,rx,ry,w,float(g[5])*sx,float(g[6])*sy,sn,cs,packed);
                        else GlyphQuad(material,g,rx,ry,w,sx,sy,sn,cs,packed);
                    }
                    float right=float(g[4])*sx+x;
                    if(redact||underline){
                        lastAlpha=glyphAlpha;
                        Bounds&bounds=redact?red[redCount]:under[underCount];
                        if(rx<bounds.left)bounds.left=rx;
                        if(right>bounds.right)bounds.right=right;
                        if(R<U16>(g,0)!=32){if(y>maxY)maxY=y;if(y-float(g[6])<minY)minY=y-float(g[6]);}
                    }
                    if(rx<minX)minX=rx;
                    if(right>maxX)maxX=right;
                    if((flags&2)&&count==cursor){
                        float cx,cy;Rotate(x,y,baseX,baseY,sn,cs,cx,cy);
                        Loop_Cursor(material,cursorLetter,cx,cy,sn,cs,font,sx,sy,packed);
                    }
                }else if(!(flags&0x100)||specialColor){
                    U32 gc;
                    if(flags&0x20)gc=WithAlpha(glowColor,packed>>24);
                    else{
                        gc=packed&0xff000000u;
                        for(unsigned i=0;i<24;i+=8)gc|=U32(Byte(int(float(Byte(packed>>i))*0.06f)))<<i;
                    }
                    float dx=float(g[5])*(-0.375f),dy=float(g[6])*(-0.0625f);
                    for(int i=0;i<8;i+=2){
                        if(!visible)continue;
                        float rx,ry;
                        Rotate((Loop_GlowOffsets[i]+Loop_GlowOffsets[i]+dx)*sx+(ox+x),
                               (Loop_GlowOffsets[i+1]+Loop_GlowOffsets[i+1]+dy)*sy+(oy+y),
                               baseX,baseY,sn,cs,rx,ry);
                        if(extra)Loop_Slice(fxGlow,atlas,rx,ry,w,float(g[5])*sx,float(g[6])*sy,sn,cs,gc);
                        else GlyphQuad(glow,g,rx,ry,w,sx,sy,sn,cs,gc,1.75f,1.125f);
                    }
                }
            }
            x+=((flags&1)?mono:float(g[4]))*sx;
            if(flags&0x80)x+=sx*padding;
            ++count;--remaining;
        }
        if((flags&2)&&count==cursor){
            float rx,ry;Rotate(x,y,baseX,baseY,sn,cs,rx,ry);
            Loop_Cursor(material,cursorLetter,rx,ry,sn,cs,font,sx,sy,baseColor);
        }
    }
    if(flags&0x4000){
        U32 packed=U32(Min(lastAlpha*16,255))<<24;
        U32 seed=fx.seed;
        for(int i=0;i<redCount;++i){
            int t=Min(Elapsed(birth),redactStart+redactDuration);
            const Byte*m=Loop_RedactionMaterials[Rand(seed)%3];
            if(t<redactStart)continue;
            float start= minX-red[i].left>=0?minX:red[i].left;
            float end=(maxX-minX)*(float(t-redactStart)/float(redactDuration))+minX;
            if(red[i].right-end<0)end=red[i].right;
            float fraction=(end-start)/(red[i].right-red[i].left);
            if(fraction>0||fraction<0)Loop_Stretch(m,start,minY,end-start,maxY-minY,0,0,fraction,1,packed);
        }
        for(int i=0;i<underCount;++i)Loop_Stretch(Loop_RedactionMaterials[0],under[i].left,minY,under[i].right-under[i].left,maxY-minY,0,0,1,1,packed);
    }
}

}
#endif
