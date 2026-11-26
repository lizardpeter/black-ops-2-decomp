// Store MP LUI text backend, original 1406AF8F0 (1373 instructions).
// Candidate: cache, direct glyph path and wrapped path retain distinct original
// passes. Word-boundary state and unusual malformed-control cases need replay.
#define T6_SOURCE "lui/lui_render.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_lui_text{
using Byte=unsigned char;
std::uint32_t DecodeCharacter(const Byte**);
void ColorForCode(Byte,float*);
void RectVertices(float,float,float,float,float*);
void SubmitQuadChain(void*,Byte*);
void ReleaseQuadChain(void*,Byte*);
}
namespace store_lui_icon{
unsigned char* DrawInlineIcon(int,void*,void*,const char*,void*,const unsigned char*,float*,float,float,const float*,unsigned char*);
}
namespace store_localize{bool ReplaceBinding(int,unsigned*,unsigned*,char*);}
namespace store_lui_render{
using B=unsigned char;using U=std::uint32_t;using H=std::uint16_t;
template<class T>static T R(const void*p,unsigned off){T v;std::memcpy(&v,static_cast<const B*>(p)+off,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof(v));}
extern "C"{
extern B LuiRender_NoCache; // 1443B1F46
extern const B* LuiRender_LocalizeDvar; // 142121160
extern void* LuiRender_DefaultMaterial; // 1428382C8
extern char LuiRender_IconScratch[65]; // 1443C5530
extern int LuiRender_AllocCount,LuiRender_AllocBytes,LuiRender_Layer;
int LuiRender_Atoi(const char*);
int LuiRender_IsSpace(int);
const char* LuiRender_Translate(const char*,const char*,int); // 1403E9320
void LuiRender_PackColor(const float*,B*);
const B* LuiRender_Glyph(const B*,U);
int LuiRender_Width(int,const char*,int,const B*);
void* LuiRender_Material(const char*,int,bool,int);
void* LuiRender_Allocate(void*,unsigned);
void LuiRender_GetField(void*,int,const char*);
void LuiRender_SetMeta(void*,int);
void LuiRender_Unref(void*,int,int);
int LuiRender_Ref(void*,int);
void LuiRender_Submit(const void*,short,const void*,const void*,const void*,int);
}
enum class Kind{End,Glyph,Icon};
struct Token{Kind kind;U ch;void*material;};
static Token NextToken(const B*&p,U*packed,const float*base){
    while(*p){
        if(*p==20||*p==21){++p;continue;}
        if(*p!='^')return {Kind::Glyph,store_lui_text::DecodeCharacter(&p),nullptr};
        B code=p[1];
        if(!code){++p;break;}
        if(code=='B'){
            p+=2;unsigned n=0;
            while(*p!='^'&&n<64)LuiRender_IconScratch[n++]=char(*p++);
            LuiRender_IconScratch[n]=0;
            U c=store_lui_text::DecodeCharacter(&p);
            return {Kind::Icon,c,nullptr};
        }
        if(code=='H'||code=='I'){
            ++p;int n=static_cast<signed char>(p[3]);
            char name[256];std::memcpy(name,p+4,n);name[n]=0;
            void*m=*name?LuiRender_Material(name,7,true,-1):LuiRender_DefaultMaterial;
            unsigned i=0;
            while(*p!='^'&&i<3)LuiRender_IconScratch[i++]=char(*p++);
            LuiRender_IconScratch[i]=0;p+=n+1;
            return {Kind::Icon,0,m};
        }
        if(packed&&B(code-'0')<10){
            if(code=='7')LuiRender_PackColor(base,reinterpret_cast<B*>(packed));
            else{
                float rgba[4];store_lui_text::ColorForCode(code,rgba);
                U a=*packed&0xff000000u;
                LuiRender_PackColor(rgba,reinterpret_cast<B*>(packed));*packed=(*packed&0xffffffu)|a;
            }
        }
        p+=2;
    }
    return {Kind::End,0,nullptr};
}
// Original inlined pair search has inclusive upper bound and short-circuits
// on the first equal pair; preserve the unusual same-first branch.
static int Kern(const B*f,U prev,U c){
    int lo=0,hi=R<int>(f,0x14);const B*table=R<const B*>(f,0x30);
    while(lo<=hi){
        int mid=(lo+hi)/2;const B*p=table+mid*8;
        U a=R<H>(p,0);
        if(a==prev){U b=R<H>(p,2);if(b==c)return R<int>(p,4);if(b<c)lo=mid+1;hi=mid-1;}
        else if(a<prev)lo=mid+1;else hi=mid-1;
    }
    return 0;
}
struct CacheWriter{B*root,*tail,*vertices,*uv,*colors;short written;};
static CacheWriter SetupCache(void*lua,B*element,const B*font,B*old,short count){
    B*node=old;
    if(!LuiRender_NoCache){
        if(!old||R<short>(old,0x20)<count){
            unsigned bytes=unsigned(int(count)*0x54+0x28);
            node=static_cast<B*>(LuiRender_Allocate(lua,bytes));
            ++LuiRender_AllocCount;LuiRender_AllocBytes+=int(bytes);
            W(node,0x24,old?R<int>(old,0x24):-2);
            LuiRender_GetField(lua,-10000,"UIQuadCache");LuiRender_SetMeta(lua,-2);
            if(R<int>(element,0x120)!=-2)LuiRender_Unref(lua,-10000,R<int>(element,0x120));
            W(element,0x120,LuiRender_Ref(lua,-10000));W(element,0x128,node);
            W(node,0x24,-2);
        }
        W(node,0x18,R<const void*>(font,0x18));W(node,0,node+0x28);
        W(node,8,node+0x28+int(count)*0x30);W(node,0x10,node+0x28+int(count)*0x50);
        node[0x22]=0;W(node,0x20,count);
    }
    if(!node)return {nullptr,nullptr,nullptr,nullptr,nullptr,0};
    return {node,node,R<B*>(node,0),R<B*>(node,8),R<B*>(node,0x10),0};
}
static void EmitGlyph(CacheWriter&out,const B*font,U ch,U&previous,float&x,float y,float scale,U packed){
    const B*g=LuiRender_Glyph(font,ch);
    x+=float(Kern(font,previous,ch))*scale;
    float left=float(static_cast<signed char>(g[2]))*scale+x;
    float top=float(static_cast<signed char>(g[3]))*scale+y;
    float right=float(g[5])*scale+left,bottom=float(g[6])*scale+top;
    float s0=R<float>(g,8),t0=R<float>(g,12),s1=R<float>(g,16),t1=R<float>(g,20);
    float pos[12],uv[8]={s0,t0,s1,t0,s0,t1,s1,t1};
    store_lui_text::RectVertices(left,top,right,bottom,pos);
    if(out.root){
        std::memcpy(out.vertices,pos,48);std::memcpy(out.uv,uv,32);W(out.colors,0,packed);
        out.vertices+=48;out.uv+=32;out.colors+=4;
    }else LuiRender_Submit(R<const void*>(font,0x18),1,pos,uv,&packed,LuiRender_Layer);
    previous=ch;x+=float(g[4])*scale;++out.written;
}
static void EmitToken(CacheWriter&out,void*lua,const B*font,const Token&t,U&previous,float&x,float y,float scale,U packed,const float*rgba){
    if(t.kind==Kind::Icon)out.tail=store_lui_icon::DrawInlineIcon(0,nullptr,lua,LuiRender_IconScratch,t.material,font,&x,y,scale,rgba,out.tail);
    else if(t.kind==Kind::Glyph)EmitGlyph(out,font,t.ch,previous,x,y,scale,packed);
}
static bool Delimiter(const Token&t){return t.kind==Kind::End||(t.kind==Kind::Glyph&&t.ch<256&&(t.ch==10||LuiRender_IsSpace(int(t.ch))));}
static int RoundWidth(float w){return _mm_cvttss_si32(_mm_set_ss(w+0.5f));}
void DrawText(int client,B*element,float x,float y,float r,float g,float b,float a,
              const char*text,const B*font,float height,float wrapWidth,int alignment,void*lua){
    float rgba[4]={r,g,b,a};U packed;
    B*cache=LuiRender_NoCache?nullptr:R<B*>(element,0x128);
    LuiRender_PackColor(rgba,reinterpret_cast<B*>(&packed));
    if(cache&&!cache[0x22]){store_lui_text::SubmitQuadChain(lua,cache);return;}
    if(LuiRender_LocalizeDvar){
        const B*d=LuiRender_LocalizeDvar;
        if(R<int>(d,0x18)==1?d[0x20]!=0:LuiRender_Atoi(R<const char*>(d,0x20))!=0)
            text=LuiRender_Translate(text,"UI string",0);
    }
    char replaced[1184];unsigned len=0;while(text[len])++len;
    if(len+1<=1024){
        std::memcpy(replaced,text,len);replaced[len]=0;unsigned i=0;
        while(store_localize::ReplaceBinding(client,&i,&len,replaced)){}
        text=replaced;
    }
    const B*start=reinterpret_cast<const B*>(text),*scan=start;
    short glyphs=0,icons=0;
    for(;;){
        Token t=NextToken(scan,nullptr,rgba);
        if(t.kind==Kind::End)break;
        if(t.kind==Kind::Icon)++icons;else if(t.ch!=10)++glyphs;
    }
    CacheWriter out=SetupCache(lua,element,font,cache,glyphs);
    float scale=height/float(R<int>(font,8));
    if(glyphs||icons){
        if(!(wrapWidth>=0)){
            const B*p=start;U previous=0;
            for(;;){
                Token t=NextToken(p,&packed,rgba);if(t.kind==Kind::End)break;
                if(t.kind==Kind::Glyph&&t.ch==10)continue;
                EmitToken(out,lua,font,t,previous,x,y,scale,packed,rgba);
            }
        }else{
            // Word scan and draw are deliberately separate. The original
            // measures each word with R_TextWidth and counts tokens, not bytes.
            const B*draw=start,*word=start,*cursor=start;
            int wordChars=0,lineChars=0;
            float available=wrapWidth,lineWidth=0;
            U drawColor=packed;
            for(;;){
                const B*boundary=cursor;
                Token t=NextToken(cursor,nullptr,rgba);
                if(!Delimiter(t)){++wordChars;continue;}
                float measured=wordChars>0?float(LuiRender_Width(client,reinterpret_cast<const char*>(word),wordChars,font))*scale:0.0f;
                bool overflow=wrapWidth>available&&RoundWidth(measured)>RoundWidth(available);
                const bool end=t.kind==Kind::End,hardBreak=end||t.ch==10;
                if(overflow){
                    lineWidth=wrapWidth-available;available=wrapWidth;
                    const B*skip=word;NextToken(skip,nullptr,rgba);word=skip;--wordChars;
                }else{
                    lineChars+=wordChars;
                }
                available-=measured;
                if(hardBreak){
                    if(overflow){cursor=boundary;available=wrapWidth;}
                    else{lineWidth=wrapWidth-available;word=cursor;wordChars=0;available=wrapWidth;}
                }
                if(overflow||hardBreak){
                    float dx=x;
                    if(alignment==2)dx=(wrapWidth*0.5f+x)-lineWidth*0.5f;
                    else if(alignment==3)dx=(x+wrapWidth)-lineWidth;
                    U previous=0;
                    for(int i=0;i<lineChars;++i){
                        Token glyph=NextToken(draw,&drawColor,rgba);
                        if(glyph.kind==Kind::End)break;
                        EmitToken(out,lua,font,glyph,previous,dx,y,scale,drawColor,rgba);
                    }
                    draw=word;y+=height;lineChars=hardBreak?0:wordChars;
                    drawColor=packed;
                }
                if(!hardBreak){word=boundary;wordChars=1;}
                if(end&&wordChars==0)break;
            }
            if(out.root)W(out.root,0x20,out.written);
        }
    }
    if(out.tail)store_lui_text::ReleaseQuadChain(lua,out.tail);
    if(out.root)store_lui_text::SubmitQuadChain(lua,out.root);
}
}
#endif
