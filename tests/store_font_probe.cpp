#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/ui/ui_font_assets_store.cpp"
#include "../src/client/screen_placement_store.cpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
using U=std::uint64_t;
#define MS __attribute__((ms_abi))
namespace store_font {
static Byte language[48],resolution[48],table[72],placements[4*0x7c],views[3*0x12d0],place[32];
static char formatted[128],langString[32],resString[32];
static Byte* imageBase;
struct Trace{U count;U events[40][6];U assets[21];U result;};static Trace trace;
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
static U Hash(const char*s){U v=0;for(;*s;++s)v=v*131+Byte(*s);return v;}
static void Log(U tag,U a=0,U b=0,U c=0,U d=0,U e=0){assert(trace.count<40);auto*p=trace.events[trace.count++];p[0]=tag;p[1]=a;p[2]=b;p[3]=c;p[4]=d;p[5]=e;}
extern "C" {
const Byte *Font_Language,*Font_Resolution;
void *Font_Assets[21];
int Font_Capture;float Font_CaptureX,Font_CaptureY,Font_HudScale;
const Byte *Font_ClientTable=table,*Font_Placements=placements+0x7c,*Font_ViewSizes=views;
int Font_Atoi(const char*s){Log(1,Hash(s));return std::atoi(s);}
const char*Font_Format(const char*f,int i,const char*s){Log(2,Hash(f),U(std::uint32_t(i)),Hash(s));std::snprintf(formatted,sizeof(formatted),f,i,s);return formatted;}
void*Font_Register(const char*s,int t){Log(3,Hash(s),t);return reinterpret_cast<void*>(0x10000+trace.count*16);}
void*Font_Material(const char*s,int t,bool b,int n){Log(4,Hash(s),t,b,U(std::uint32_t(n)));return reinterpret_cast<void*>(0x20000+trace.count*16);}
}
static int MS MAtoi(const char*s){return Font_Atoi(s);}
static const char* MS MFormat(const char*f,int i,const char*s){return Font_Format(f,i,s);}
static void* MS MRegister(const char*s,int t){return Font_Register(s,t);}
static void* MS MMaterial(const char*s,int t,bool b,int n){return Font_Material(s,t,b,n);}
static const Byte* Dvar(Byte*d,char*buf,unsigned t,int v){
    if(t==0)return nullptr;
    W(d,0x18,t==1?1:t==2?2:t==3?6:t==4?7:8);
    if(t==1)W(d,0x20,Byte(v));else if(t==2)W(d,0x20,float(v)+0.25f);
    else if(t==3||t==4)W(d,0x20,v);else{std::snprintf(buf,32,"%d",v);W(d,0x20,static_cast<const char*>(buf));}
    return d;
}
extern "C" {
void* FontMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MFormat),reinterpret_cast<void*>(MRegister),reinterpret_cast<void*>(MMaterial)};return m[i];}
unsigned FontSize(){return sizeof(Trace);}
void FontRun(void*image,unsigned op,unsigned seed,void*out){
    trace={};imageBase=static_cast<Byte*>(image);
    const int langs[]={0,11,13,5,-1,255};const int res[]={720,1080,0,1440,-4,2160,1};
    Font_Language=Dvar(language,langString,seed%6,langs[(seed>>3)%6]);
    Font_Resolution=Dvar(resolution,resString,(seed>>7)%6,res[(seed>>10)%7]);
    Font_Capture=seed&1;Font_CaptureX=0.25f+float((seed>>1)&15)*0.125f;Font_CaptureY=0.5f+float((seed>>5)&15)*0.25f;
    Font_HudScale=0.125f+float((seed>>9)&31)*0.125f;
    const int client=(seed>>14)%3;
    const float val=float(int(seed%8192)-4096)*0.125f;
    for(unsigned i=0;i<3;++i){W(table,i*24,int((seed&(1u<<(17+i)))?i:99));W(table,i*24+8,int((seed>>(20+i*2))%3));}
    for(unsigned i=0;i<4;++i){W(placements,i*0x7c,0.5f+float(i));W(placements,i*0x7c+4,0.25f+float(i));}
    for(unsigned i=0;i<3;++i){W(views,i*0x12d0,1280.0f+640*i);W(views,i*0x12d0+4,720.0f+360*i);}
    W(place,0x10,0.25f);W(place,0x14,0.75f);
    for(unsigned i=0;i<21;++i)Font_Assets[i]=reinterpret_cast<void*>(0x30000+i*16);
    float result=0;
    if(image){
        auto*b=imageBase;W(b,0x2121178,Font_Language);W(b,0x27e71d0,Font_Resolution);
        W(b,0x27f1b60,Font_Capture);W(b,0x27f1b64,Font_CaptureX);W(b,0x27f1b68,Font_CaptureY);W(b,0xe6287c,Font_HudScale);
        std::memcpy(b+0x2126570,Font_Assets,sizeof(Font_Assets));std::memcpy(b+0x1ef91e4,table,sizeof(table));
        std::memcpy(b+0xe63390-0x7c,placements,sizeof(placements));std::memcpy(b+0x2c64a70,views,sizeof(views));
        switch(op){
        case 0:reinterpret_cast<void(MS*)()>(b+0x3fbaf0)();break;
        case 1:result=reinterpret_cast<float(MS*)()>(b+0xd0050)();break;
        case 2:result=reinterpret_cast<float(MS*)(int,float)>(b+0xa9ab0)(client,val);break;
        case 3:result=reinterpret_cast<float(MS*)(int,float)>(b+0xa9b20)(client,val);break;
        case 4:result=reinterpret_cast<float(MS*)(const Byte*,float)>(b+0xddb40)(place,val);break;
        case 5:result=reinterpret_cast<float(MS*)(const Byte*,float)>(b+0xd7110)(place,val);break;
        }
        std::memcpy(Font_Assets,b+0x2126570,sizeof(Font_Assets));
    }else switch(op){
        case 0:AssetCache();break;case 1:result=ResolutionScale();break;
        case 2:result=HudViewX(client,val);break;case 3:result=HudViewY(client,val);break;
        case 4:result=VirtualFromRealWidth(place,val);break;case 5:result=VirtualFromRealHeight(place,val);break;
    }
    std::memcpy(trace.assets,Font_Assets,sizeof(Font_Assets));std::memcpy(&trace.result,&result,4);std::memcpy(out,&trace,sizeof(trace));
}
}
}
