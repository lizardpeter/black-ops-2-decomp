#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/r_font_store.cpp"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#define MS __attribute__((ms_abi))
namespace store_font_core {
using U=std::uint64_t;
static Byte font[64],glyphs[160*24],dvars[3][48],placement[32];
static char strings[3][32],text[512];
struct Capture{U count;U events[32][6];U result;};static Capture cap;
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<Byte*>(p)+off,&v,sizeof(v));}
static U Hash(const char*s){U h=0;for(;*s;++s)h=h*131+Byte(*s);return h;}
static void Log(U n,U a=0,U b=0,U c=0,U d=0){assert(cap.count<32);auto*p=cap.events[cap.count++];p[0]=n;p[1]=a;p[2]=b;p[3]=c;p[4]=d;}
extern "C" {
const Byte *Core_Small,*Core_Big,*Core_ExtraBig;void *Core_Assets[21];
int Core_Capture;float Core_CaptureY,Core_HudScale;
double Core_Atof(const char*s){Log(1,Hash(s));return std::atof(s);}
void *Core_FindAsset(int t,const char*s,bool b,int n){Log(2,t,Hash(s),b,U(U32(n)));return reinterpret_cast<void*>(0x123456);}
void Core_Icon(int client,const char*s,int h,void**m,float*w,float*y){Log(3,U(U32(client)),Hash(s),U(U32(h)));*m=reinterpret_cast<void*>(0x543210);*w=1.25f;*y=0.5f;}
}
static double MS MAtof(const char*s){return Core_Atof(s);}
static void* MS MFind(int t,const char*s,bool b,int n){return Core_FindAsset(t,s,b,n);}
static void MS MIcon(int c,const char*s,int h,void**m,float*w,float*y){Core_Icon(c,s,h,m,w,y);}
extern "C" {
void* CoreMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtof),reinterpret_cast<void*>(MFind),reinterpret_cast<void*>(MIcon)};return m[i];}
unsigned CoreSize(){return sizeof(Capture);}
void CoreRun(void*image,unsigned op,unsigned seed,U32 bits,void*out){
    cap={};unsigned state=seed+1;auto next=[&](){state=state*1664525u+1013904223u;return state;};
    float scale;std::memcpy(&scale,&bits,4);
    const Byte**slots[]={&Core_Small,&Core_Big,&Core_ExtraBig};
    for(unsigned i=0;i<3;++i){
        unsigned t=(seed>>(i*2))&3;*slots[i]=t?dvars[i]:nullptr;
        W(dvars[i],0x18,t==1?2:t==2?6:8);
        int v=int(next()%17)-8;
        if(t==1)W(dvars[i],0x20,float(v)*0.25f);
        else if(t==2)W(dvars[i],0x20,v);
        else{std::snprintf(strings[i],32,"%d",v);W(dvars[i],0x20,static_cast<const char*>(strings[i]));}
    }
    for(unsigned i=0;i<21;++i)Core_Assets[i]=reinterpret_cast<void*>(0x30000+i*16);
    Core_Capture=(seed>>6)&1;Core_CaptureY=float((seed>>7)&15)*0.25f;Core_HudScale=float((seed>>11)&15)*0.25f;
    W(placement,4,float((seed>>15)&15)*0.25f);
    int kind=int((seed>>19)%13)-2;
    unsigned count=96+next()%65;
    W(font,0x10,int(count));W(font,0x28,static_cast<const Byte*>(glyphs));W(font,8,int(next()%100+1));
    for(unsigned i=0;i<160;++i){W(glyphs,i*24,std::uint16_t(i<96?i+32:0x80+(i-96)*7));glyphs[i*24+4]=Byte(next()>>24);}
    U32 letters[]={0,1,31,32,46,65,127,128,135,0x1234,0xffff,0x10000,0xffffffffu};
    U32 letter=seed&1?letters[next()%13]:next()%600;
    const char*parts[]={"a","XYZ","^1","^C2","^L","^F","^^","^X","\n","\r","\x14","\x15","\xc2\x80","\xe1\x80\x80","^Bicon^","^B^"," ","Z"};
    unsigned n=next()%24;std::memset(text,0,sizeof(text));unsigned pos=0;
    for(unsigned i=0;i<n;++i){auto*s=parts[next()%18];unsigned l=unsigned(std::strlen(s));std::memcpy(text+pos,s,l);pos+=l;}
    int max=int(next()%32)-3,client=int(next()%3)-1;
    if(image){
        auto*b=static_cast<Byte*>(image);W(b,0x2121448,Core_Small);W(b,0x2121368,Core_Big);W(b,0x21213b8,Core_ExtraBig);
        std::memcpy(b+0x2126570,Core_Assets,sizeof(Core_Assets));W(b,0x27f1b60,Core_Capture);W(b,0x27f1b68,Core_CaptureY);W(b,0xe6287c,Core_HudScale);
        switch(op){
        case 0:cap.result=reinterpret_cast<U>(reinterpret_cast<void*(MS*)(float)>(b+0x3fcd20)(scale));break;
        case 1:cap.result=reinterpret_cast<U>(reinterpret_cast<void*(MS*)(const Byte*,int,float)>(b+0x3fce40)(placement,kind,scale));break;
        case 2:cap.result=U(reinterpret_cast<const Byte*(MS*)(const Byte*,U32)>(b+0x71a6e0)(font,letter)-glyphs);break;
        case 3:cap.result=reinterpret_cast<U>(reinterpret_cast<void*(MS*)(const char*)>(b+0x71a780)("fonts/720/normalfont"));break;
        case 4:cap.result=U(U32(reinterpret_cast<int(MS*)(int,const char*,int,const Byte*)>(b+0x71a7a0)(client,text,max,font)));break;
        }
    }else switch(op){
        case 0:cap.result=reinterpret_cast<U>(DefaultFont(scale));break;
        case 1:cap.result=reinterpret_cast<U>(FontHandle(placement,kind,scale));break;
        case 2:cap.result=U(CharacterGlyph(font,letter)-glyphs);break;
        case 3:cap.result=reinterpret_cast<U>(RegisterFont("fonts/720/normalfont"));break;
        case 4:cap.result=U(U32(TextWidth(client,text,max,font)));break;
    }
    std::memcpy(out,&cap,sizeof(cap));
}
}
}
