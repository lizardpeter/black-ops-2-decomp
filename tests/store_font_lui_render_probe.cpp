#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/lui_text_util_store.cpp"
#undef T6_SOURCE
#include "../src/ui/lui_inline_icon_store.cpp"
#undef T6_SOURCE
#include "../src/ui/lui_text_render_store.cpp"
#include <cassert>
#include <cstdlib>
#include <cctype>
#define MS __attribute__((ms_abi))
namespace {
using B=unsigned char;using U=std::uint32_t;using Q=std::uint64_t;
template<class T>void Put(void*p,unsigned o,T v){std::memcpy(static_cast<B*>(p)+o,&v,sizeof(v));}
template<class T>T Get(const void*p,unsigned o){T v;std::memcpy(&v,static_cast<const B*>(p)+o,sizeof(v));return v;}
struct Event{Q material;int count,layer;float pos[32*12],uv[32*8];U color[32];};
struct Trace{unsigned count;float x;int result;Event events[128];};
static Trace trace;
static B font[64],glyphs[96*24],pairs[16*8],element[0x140],node[0x28+32*84],lua[128];
static B node2[0x7c],luaStack[64];
static char text[1024];
static void Pack(const float*rgba,B*out){
    for(unsigned i=0;i<4;++i){
        int v=int(rgba[i]*255.0f+0.5f);
        out[i]=B(v<0?0:v>255?255:v);
    }
}
static void Submit(const void*material,short n,const void*pos,const void*uv,const void*colors,int layer){
    assert(n>=1&&n<=32&&trace.count<128);
    auto&e=trace.events[trace.count++];e.material=reinterpret_cast<Q>(material);e.count=n;e.layer=layer;
    std::memcpy(e.pos,pos,n*48);std::memcpy(e.uv,uv,n*32);std::memcpy(e.color,colors,n*4);
}
static void Info(int,const char*,int,void**m,float*w,float*h){*m=reinterpret_cast<void*>(0x5678);*w=1.25f;*h=0.75f;}
static void* Material(const char*,int,bool,int){return reinterpret_cast<void*>(0x4567);}
static const B* Glyph(const B*f,U c){return Get<const B*>(f,0x28)+24*((c-32u)<=95?c-32u:14);}
static int Width(int,const char*,int n,const B*){return n*12;}
static void* Alloc(void*,unsigned){return node2;}
static int Ref(void*,int){return 99;}
static void Unref(void*,int,int){}
static void RawGet(void*,int,int){}
static void* UserData(void*,int){return node2;}
static int MS MAtoi(const char*s){return std::atoi(s);}
static int MS MSpace(int c){return std::isspace(c);}
static void MS MPack(const float*c,B*out){Pack(c,out);}
static void MS MSubmit(const void*m,short n,const void*p,const void*u,const void*c,int l){Submit(m,n,p,u,c,l);}
static void MS MInfo(int c,const char*s,int n,void**m,float*w,float*h){Info(c,s,n,m,w,h);}
static void* MS MMaterial(const char*s,int t,bool a,int z){return Material(s,t,a,z);}
static int MS MWidth(int c,const char*s,int n,const B*f){return Width(c,s,n,f);}
static bool MS MReplace(int,unsigned*,unsigned*,char*){return false;}
static void* MS MAlloc(void*l,unsigned n){return Alloc(l,n);}
static int MS MRef(void*l,int n){return Ref(l,n);}
static void MS MUnref(void*l,int n,int v){Unref(l,n,v);}
static void MS MRawGet(void*l,int n,int v){RawGet(l,n,v);}
static void* MS MUserData(void*l,int n){return UserData(l,n);}
}
namespace store_lui_text{
extern "C"{
float Lui_ColorTable[8][4],Lui_Color8[4],Lui_Color9[4];
int Lui_TransformDepth;float Lui_TransformStack[4][16];const Byte*Lui_Language;
float Lui_AlignFactors[4];int Lui_Layer;
int Lui_Atoi(const char*s){return std::atoi(s);}
void Lui_PushRegistry(void*l,int n,int v){RawGet(l,n,v);}
void*Lui_ToUserData(void*l,int n){return UserData(l,n);}
void Lui_Unref(void*l,int n,int v){Unref(l,n,v);}
void Lui_AddQuadArray(const void*m,short n,const void*p,const void*u,const void*c,int l){Submit(m,n,p,u,c,l);}
}
}
namespace store_lui_icon{
extern "C"{
int Icon_Layer,Icon_AllocCount,Icon_AllocBytes;
void Icon_Info(int c,const char*s,int n,void**m,float*w,float*h){Info(c,s,n,m,w,h);}
void Icon_PackColor(const float*c,B*out){Pack(c,out);}
void Icon_Submit(const void*m,short n,const void*p,const void*u,const void*c,int l){Submit(m,n,p,u,c,l);}
void Icon_RawGet(void*l,int n,int v){RawGet(l,n,v);}
void*Icon_UserData(void*l,int n){return UserData(l,n);}
void*Icon_Alloc(void*l,int n){return Alloc(l,unsigned(n));}
void Icon_Unref(void*l,int n,int v){Unref(l,n,v);}
int Icon_Ref(void*l,int n){return Ref(l,n);}
}
}
namespace store_localize{bool ReplaceBinding(int,unsigned*,unsigned*,char*){return false;}}
namespace store_lui_render{
extern "C"{
B LuiRender_NoCache;const B*LuiRender_LocalizeDvar;void*LuiRender_DefaultMaterial;
char LuiRender_IconScratch[65];int LuiRender_AllocCount,LuiRender_AllocBytes,LuiRender_Layer;
int LuiRender_Atoi(const char*s){return std::atoi(s);}
int LuiRender_IsSpace(int c){return std::isspace(c);}
const char*LuiRender_Translate(const char*s,const char*,int){return s;}
void LuiRender_PackColor(const float*c,B*out){Pack(c,out);}
const B*LuiRender_Glyph(const B*f,U c){return Glyph(f,c);}
int LuiRender_Width(int c,const char*s,int n,const B*f){return Width(c,s,n,f);}
void*LuiRender_Material(const char*s,int n,bool a,int z){return Material(s,n,a,z);}
void*LuiRender_Allocate(void*l,unsigned n){return Alloc(l,n);}
void LuiRender_GetField(void*,int,const char*){}
void LuiRender_SetMeta(void*,int){}
void LuiRender_Unref(void*l,int n,int v){Unref(l,n,v);}
int LuiRender_Ref(void*l,int n){return Ref(l,n);}
void LuiRender_Submit(const void*m,short n,const void*p,const void*u,const void*c,int l){Submit(m,n,p,u,c,l);}
}
}
extern "C"{
void*LuiRenderMock(unsigned i){
    void*p[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MSpace),reinterpret_cast<void*>(MPack),reinterpret_cast<void*>(MSubmit),reinterpret_cast<void*>(MInfo),reinterpret_cast<void*>(MMaterial),reinterpret_cast<void*>(MWidth),reinterpret_cast<void*>(MReplace),reinterpret_cast<void*>(MAlloc),reinterpret_cast<void*>(MRef),reinterpret_cast<void*>(MUnref),reinterpret_cast<void*>(MRawGet),reinterpret_cast<void*>(MUserData)};
    return p[i];
}
unsigned LuiRenderSize(){return sizeof(Trace);}
void LuiRenderRun(void*image,unsigned op,unsigned seed,void*out){
    using namespace store_lui_text;using namespace store_lui_icon;using namespace store_lui_render;
    trace={};auto*b=static_cast<B*>(image);U st=seed+1;
    auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%2048)-1024)*0.125f;};
    Lui_TransformDepth=0;Lui_Language=nullptr;
    Lui_Layer=Icon_Layer=LuiRender_Layer=int(next()%300)-150;
    LuiRender_LocalizeDvar=nullptr;LuiRender_DefaultMaterial=reinterpret_cast<void*>(0x9876);
    LuiRender_NoCache=op==2||op==3?0:1;
    for(auto&c:Lui_ColorTable)for(float&v:c)v=float(next()%256)/255.0f;
    for(float&v:Lui_Color8)v=float(next()%256)/255.0f;
    for(float&v:Lui_Color9)v=float(next()%256)/255.0f;
    Put(font,8,int(next()%64+1));Put(font,0x10,96);Put(font,0x14,15);
    Put(font,0x18,reinterpret_cast<void*>(0x1234));Put(font,0x28,static_cast<const B*>(glyphs));Put(font,0x30,static_cast<const B*>(pairs));
    for(unsigned i=0;i<96;++i){
        Put(glyphs,i*24,std::uint16_t(i+32));
        for(unsigned j=2;j<7;++j)glyphs[i*24+j]=B(next()>>24);
        for(unsigned j=8;j<24;j+=4)Put(glyphs,i*24+j,f());
    }
    for(unsigned i=0;i<16;++i){Put(pairs,i*8,std::uint16_t(i/4*8+65));Put(pairs,i*8+2,std::uint16_t(i%4+65));Put(pairs,i*8+4,int(next()%17)-8);}
    std::memset(element,0,sizeof(element));std::memset(node,0,sizeof(node));std::memset(node2,0,sizeof(node2));
    Put(element,0x120,-2);Put(element,0x128,node);Put(node,0x24,-2);Put(node,0x20,short(32));node[0x22]=op==2?0:1;
    Put(node,0,node+0x28);Put(node,8,node+0x28+32*48);Put(node,0x10,node+0x28+32*80);Put(node,0x18,reinterpret_cast<void*>(0x1234));
    Put(lua,0x48,luaStack+32);
    float x=f(),y=f(),scale=float(next()%32+1)*0.125f,rgba[4];
    for(float&v:rgba)v=float(next()%256)/255.0f;
    if(b){
        Put(b,0x43b1f46,LuiRender_NoCache);Put(b,0x2121160,LuiRender_LocalizeDvar);Put(b,0x28382c8,LuiRender_DefaultMaterial);Put(b,0x43c551c,LuiRender_Layer);
        Put(b,0x43c2bd4,Lui_TransformDepth);Put(b,0x2121178,Lui_Language);
        std::memcpy(b+0x8e6870,Lui_ColorTable,sizeof(Lui_ColorTable));std::memcpy(b+0x3c11c00,Lui_Color8,16);std::memcpy(b+0x3c11c10,Lui_Color9,16);
    }
    if(op==0){
        char tag[5]={'H',33,63,0,0};if(seed&1)tag[0]='I';
        const char*name=seed&2?"test":tag;
        void*m=seed&2?nullptr:reinterpret_cast<void*>(0x1234);
        if(b){using F=B*(MS*)(int,void*,void*,const char*,void*,const B*,float*,float,float,const float*,B*);trace.result=reinterpret_cast<F>(b+0x6af590)(0,nullptr,lua,name,m,font,&x,y,scale,rgba,nullptr)!=nullptr;}
        else trace.result=DrawInlineIcon(0,nullptr,lua,name,m,font,&x,y,scale,rgba,nullptr)!=nullptr;
        trace.x=x;
    }else{
        const char*parts[]={"A","XYZ","^1","^7","^8","^9"," ","\n","\r","\xc2\x80","^Bicon^","^H!!\x01x","ABC"};
        unsigned pos=0,n=next()%10;std::memset(text,0,sizeof(text));
        if(op==4||op==5){
            const char*strings[]={"ABC","a b c","A B C D","word word word","a\nb","A B C D E F"};
            std::strcpy(text,strings[next()%6]);
        }else{
            for(unsigned i=0;i<n;++i){const char*s=parts[next()%(op==3?10:13)];unsigned l=unsigned(std::strlen(s));std::memcpy(text+pos,s,l);pos+=l;}
        }
        float width=op==4?10000.0f:op==5?float(next()%80+20):-1.0f;
        int align=1+int(next()%3);float height=float(Get<int>(font,8))*scale;
        if(b){
            using F=void(MS*)(int,B*,float,float,float,float,float,float,const char*,const B*,float,float,int,void*);
            reinterpret_cast<F>(b+0x6af8f0)(0,element,x,y,rgba[0],rgba[1],rgba[2],rgba[3],text,font,height,width,align,lua);
        }else DrawText(0,element,x,y,rgba[0],rgba[1],rgba[2],rgba[3],text,font,height,width,align,lua);
    }
    std::memcpy(out,&trace,sizeof(trace));
}
}
