#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/rb_text_loop_store.cpp"
#include <cassert>
#define MS __attribute__((ms_abi))
namespace store_text_loop{
using U=std::uint64_t;
static Byte glyphs[96*24],font[64],pairs[16*8],text[1024];
struct Event{U tag,mat,color;float f[11];U extra;};
struct Trace{U count;Event events[512];};static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static Event& E(U tag,const void*m=nullptr,U32 color=0){
    assert(trace.count<512);auto&e=trace.events[trace.count++];e.tag=tag;e.mat=reinterpret_cast<U>(m);e.color=color;return e;
}
extern "C"{
int Loop_GameTime;U32 Loop_Palette[8],Loop_Color8,Loop_Color9;
int Loop_TimerInitialized,Loop_BlinkBase,Loop_BlinkElapsed;U32 Loop_TimerOrigin;Byte Loop_BlinkVisible,Loop_JitterPattern[176];float Loop_GlowOffsets[8];const Byte*Loop_RedactionMaterials[3];
U32 Loop_Tick(){return 1000;}
int Loop_PrintLength(const Byte*p){int n=0;while(*p++){++n;}return n;}
void Loop_Stretch(const Byte*m,float x,float y,float w,float h,float s0,float t0,float s1,float t1,U32 c){Loop_Quad(m,x,y,1,w,h,s0,t0,s1,t1,0,1,c);}
void Loop_Slice(const Byte*m,int n,float x,float y,float w,float a,float b,float sn,float cs,U32 c){float s=float(n%16)*0.0625f;Loop_Quad(m,x,y,w,a,b,s,0,s+0.0625f,1,sn,cs,c);}
const Byte*Loop_Glyph(const Byte*f,U32 c){return R<const Byte*>(f,0x28)+24*((c-32u)<=95u?c-32u:14u);}
int Loop_Width(int,const char*,int n,const Byte*){auto&e=E(4);e.extra=U(n);return n*5;}
void Loop_Quad(const Byte*m,float x,float y,float z,float w,float h,float s0,float t0,float s1,float t1,float sn,float cs,U32 color){
    auto&e=E(1,m,color);float a[]={x,y,z,w,h,s0,t0,s1,t1,sn,cs};std::memcpy(e.f,a,sizeof(a));
}
void Loop_Cursor(const Byte*m,Byte ch,float x,float y,float sn,float cs,const Byte*,float sx,float sy,U32 color){
    auto&e=E(2,m,color);float a[]={x,y,sn,cs,sx,sy};std::memcpy(e.f,a,sizeof(a));e.extra=ch;
}
float Loop_Legacy(const char*t,float x,float y,float sn,float cs,const Byte*,float sx,float sy,U32 color){
    auto&e=E(3,nullptr,color);float a[]={x,y,sn,cs,sx,sy};std::memcpy(e.f,a,sizeof(a));e.extra=Byte(t[0]);return 9.5f;
}
float Loop_Named(const char*t,float x,float y,float sn,float cs,const Byte*,float sx,float sy,U32 color,int*n){
    auto&e=E(5,nullptr,color);float a[]={x,y,sn,cs,sx,sy};std::memcpy(e.f,a,sizeof(a));
    *n=1;while(t[*n]!='^')++*n;++*n;e.extra=*n;return 12.25f;
}
}
static U32 MS MTick(){return Loop_Tick();}
static int MS MPrint(const Byte*p){return Loop_PrintLength(p);}
static void MS MStretch(const Byte*m,float x,float y,float w,float h,float s0,float t0,float s1,float t1,U32 c){Loop_Stretch(m,x,y,w,h,s0,t0,s1,t1,c);}
static void MS MSlice(const Byte*m,int n,float x,float y,float w,float a,float b,float sn,float cs,U32 c){Loop_Slice(m,n,x,y,w,a,b,sn,cs,c);}
static int MS MWidth(int c,const char*t,int n,const Byte*f){return Loop_Width(c,t,n,f);}
static void MS MQuad(const Byte*m,float x,float y,float z,float w,float h,float s0,float t0,float s1,float t1,float sn,float cs,U32 c){Loop_Quad(m,x,y,z,w,h,s0,t0,s1,t1,sn,cs,c);}
static void MS MCursor(const Byte*m,Byte ch,float x,float y,float sn,float cs,const Byte*f,float sx,float sy,U32 c){Loop_Cursor(m,ch,x,y,sn,cs,f,sx,sy,c);}
static float MS MLegacy(const char*t,float x,float y,float sn,float cs,const Byte*f,float sx,float sy,U32 c){return Loop_Legacy(t,x,y,sn,cs,f,sx,sy,c);}
static float MS MNamed(const char*t,float x,float y,float sn,float cs,const Byte*f,float sx,float sy,U32 c,int*n){return Loop_Named(t,x,y,sn,cs,f,sx,sy,c,n);}
extern "C" {
void*LoopMock(unsigned i){void*p[]={reinterpret_cast<void*>(MWidth),reinterpret_cast<void*>(MQuad),reinterpret_cast<void*>(MCursor),reinterpret_cast<void*>(MLegacy),reinterpret_cast<void*>(MNamed),reinterpret_cast<void*>(MTick),reinterpret_cast<void*>(MPrint),reinterpret_cast<void*>(MStretch),reinterpret_cast<void*>(MSlice)};return p[i];}
unsigned LoopSize(){return sizeof(Trace);}
static void Run(void*image,unsigned seed,void*out,unsigned mode){
    trace={};unsigned st=seed+1;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%2048)-1024)*0.125f;};
    W(font,0x10,96);W(font,0x14,15);W(font,0x30,static_cast<const Byte*>(pairs));W(font,0x28,static_cast<const Byte*>(glyphs));
    W(font,8,int(next()%64+1));W(font,0x18,reinterpret_cast<const Byte*>(0x12345678));
    for(unsigned i=0;i<96;++i){
        W(glyphs,i*24,U16(i+32));for(unsigned j=2;j<7;++j)glyphs[i*24+j]=Byte(next()>>24);
        for(unsigned j=8;j<24;j+=4)W(glyphs,i*24+j,f());
    }
    for(unsigned i=0;i<16;++i){W(pairs,i*8,U16(i/4*8+65));W(pairs,i*8+2,U16(i%4+65));W(pairs,i*8+4,int(next()%21)-10);}
    const char*parts[]={"a","XYZ","^1","^8","^9","^7","^F","^^","^X","\n","\r","\t","\xc2\x80","\xe1\x80\x80"," ","Z","^Bicon^","^H!!\x01x","AAA","ABCD"};
    unsigned pos=0,n=next()%30;std::memset(text,0,sizeof(text));
    for(unsigned i=0;i<n;++i){const char*s=parts[next()%20];unsigned len=unsigned(std::strlen(s));std::memcpy(text+pos,s,len);pos+=len;}
    float x=f(),y=f(),width=float(next()%2000+1)*0.125f,sx=f()*0.125f,sy=f()*0.125f;
    U32 color=next(),flags=(seed&15)|((seed&16)?0x20000u:0u);
    int cursor=int(next()%60)-2,birth=int(next()%4000),interval=int(next()%40)+1;
    Loop_GameTime=birth+(int(next()%70)-4)*interval;Byte ch=Byte(next());for(auto&c:Loop_Palette)c=next();Loop_Color8=next();Loop_Color9=next();
    if(mode){
        const char*plain[]={"ABCD","a","XYZ"," ","\n","\r","^1","^7","^8","^9","^Bicon^","^H!!\x01x"};
        pos=0;std::memset(text,0,sizeof(text));
        for(unsigned i=0;i<n;++i){const char*s=plain[next()%(mode==1?6:12)];unsigned len=unsigned(std::strlen(s));std::memcpy(text+pos,s,len);pos+=len;}
        sx=float(next()%9+1)*0.125f;sy=float(next()%9+1)*0.125f;
        flags=mode==1?0u:(seed&15)|((seed&16)?0x80u:0u);
        Loop_TimerInitialized=1;Loop_TimerOrigin=400;Loop_BlinkBase=500;Loop_BlinkElapsed=0;Loop_BlinkVisible=1;
        W(font,0x20,reinterpret_cast<const Byte*>(0x87654321));
        if(image){
            auto*b=static_cast<Byte*>(image);
            W(b,0x2c48354,Loop_GameTime);std::memcpy(b+0x9b0138,Loop_Palette,sizeof(Loop_Palette));W(b,0x27f18cc,Loop_Color8);W(b,0x27f18c8,Loop_Color9);
            W(b,0x449e974,Loop_TimerInitialized);W(b,0x21e448c,Loop_TimerOrigin);W(b,0x449ed10,Loop_BlinkBase);W(b,0x449ed0c,Loop_BlinkElapsed);W(b,0xd3af68,Loop_BlinkVisible);
            using F=void(MS*)(const Byte*,float,float,float,const Byte*,float,float,float,float,U32,int,U32,int,Byte,float,U32,int,int,int,int,int,int,const Byte*,const Byte*);
            reinterpret_cast<F>(b+0x732480)(text,x,y,width,font,sx,sy,0,1,color,100,flags,cursor,ch,0.25f,0,birth,interval,2000,300,50,100,nullptr,nullptr);
        }else DrawText2D(text,x,y,width,font,sx,sy,0,1,color,100,flags,cursor,ch,0.25f,0,birth,interval,2000,300,50,100,nullptr,nullptr);
        std::memcpy(out,&trace,sizeof(trace));return;
    }
    if(image){
        auto*b=static_cast<Byte*>(image);W(b,0x2c48354,Loop_GameTime);std::memcpy(b+0x9b0138,Loop_Palette,sizeof(Loop_Palette));W(b,0x27f18cc,Loop_Color8);W(b,0x27f18c8,Loop_Color9);
        using F=void(MS*)(const Byte*,float,float,float,float,const Byte*,float,float,U32,U32,int,Byte,int,int,int);
        reinterpret_cast<F>(b+0x734690)(text,x,y,width,0,font,sx,sy,color,flags,cursor,ch,0,birth,interval);
    }else DrawTeleType(text,x,y,width,0,font,sx,sy,color,flags,cursor,ch,0,birth,interval);
    std::memcpy(out,&trace,sizeof(trace));
}
void LoopRun(void*image,unsigned seed,void*out){Run(image,seed,out,0);}
void LoopRun2D(void*image,unsigned seed,unsigned mode,void*out){Run(image,seed,out,mode);}
}
}
