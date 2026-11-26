#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/r_text_commands_store.cpp"
#undef T6_SOURCE
#include "../src/client/screen_placement_store.cpp"
#include <cstdlib>
#include <cstdio>
#define MS __attribute__((ms_abi))
namespace store_font {
extern "C" {
int Font_Capture;
float Font_CaptureX,Font_CaptureY,Font_HudScale;
unsigned Font_CaptureWidth,Font_CaptureHeight;
float Font_CaptureOffsetX,Font_CaptureOffsetY;
const Byte *Font_ClientTable,*Font_Placements,*Font_ViewSizes;
}
}
namespace store_text_cmd {
extern "C" {
CommandList*Text_CommandList;
int Text_UI3DDepth,Text_UI3DStack[8];
Byte Text_UI3DStats[6*0xa4];
const Byte*Text_DualPlay;
const void*Text_DefaultMaterial;
int Text_Atoi(const char*s){return std::atoi(s);}
void Text_Color(const float*c,Byte*out){for(unsigned i=0;i<4;++i)out[i]=static_cast<Byte>(c[i]*255);}
}
static int MS MAtoi(const char*s){return Text_Atoi(s);}
static void MS MColor(const float*c,Byte*out){Text_Color(c,out);}
static void* MS MCopy(void*d,const void*s,std::size_t n){return std::memcpy(d,s,n);}
struct Capture{int used,last,result;unsigned char command[768],stats[6*0xa4];float rect[4];};
static Byte storage[768],dvar[48],place[0x7c];
static char text[320];
static CommandList list;
static int offset(const Byte*p){return p?int(p-storage):-1;}
extern "C" {
void* CmdMock(unsigned i){void*m[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MColor),reinterpret_cast<void*>(MCopy)};return m[i];}
unsigned CmdSize(){return sizeof(Capture);}
void CmdRun(void*image,unsigned op,unsigned seed,void*output){
    Capture cap{};auto*b=static_cast<Byte*>(image);
    unsigned state=seed+1;auto next=[&](){state=state*1664525u+1013904223u;return state;};
    auto f=[&](){return float(int(next()%2048)-1024)*0.125f;};
    if(op==0){
        std::memset(storage,0xa7,sizeof(storage));std::memset(Text_UI3DStats,0x21,sizeof(Text_UI3DStats));std::memset(dvar,0,sizeof(dvar));
        unsigned len=next()%300;if(seed&0x40)len=0;
        for(unsigned i=0;i<len;++i)text[i]=char(32+next()%95);
        text[len]=0;
        list={storage,int(next()%16)*4,int(next()%64),storage+4,0};
        list.byteSize=list.usedTotal-list.usedCritical+0x2000+int(next()%460);
        Text_CommandList=&list;Text_UI3DDepth=int(next()%3);
        for(int&i:Text_UI3DStack)i=int(next()%12)-3;
        switch(seed%5){case 0:Text_DualPlay=nullptr;break;case 1:case 2:W(dvar,0x18,1);W(dvar,0x20,Byte(seed%2));Text_DualPlay=dvar;break;
        default:W(dvar,0x18,8);W(dvar,0x20,seed%2?"1":"0");Text_DualPlay=dvar;break;}
        float x=f(),y=f(),w=f(),sx=f(),sy=f(),rot=f(),pad=f();
        float color[4]={0.25f,0.5f,0.75f,1.0f};int styles[]={0,1,3,6,0x80,-1};
        const int style=styles[next()%6],cursor=int(next()%5)-2,max=int(next()%100)-10;
        Byte letter=Byte(next());const void*font=reinterpret_cast<void*>(0x12345678);
        Byte*result;
        if(b){
            W(b,0x27e7290,&list);W(b,0x2838ae0,Text_UI3DDepth);W(b,0x27e6d70,Text_DualPlay);
            // Stack storage overlaps depth at index 3; use the real ordering and valid depth range.
            std::memcpy(b+0x2838ad4,Text_UI3DStack,3*sizeof(int));
            W(b,0x2838ae0,Text_UI3DDepth);
            for(int i=0;i<6;++i)std::memcpy(b+0x283879c+i*0xa4,Text_UI3DStats+i*0xa4,8);
            using Fn=Byte*(MS*)(const char*,int,const void*,float,float,float,float,float,float,const float*,int,int,Byte,float);
            result=reinterpret_cast<Fn>(b+0x71c880)(text,max,font,x,y,w,sx,sy,rot,color,style,cursor,letter,pad);
            for(int i=0;i<6;++i)std::memcpy(Text_UI3DStats+i*0xa4,b+0x283879c+i*0xa4,8);
        }else result=AddBaseDrawTextCmd(text,max,font,x,y,w,sx,sy,rot,color,style,cursor,letter,pad);
        cap.used=list.usedTotal;cap.last=offset(list.lastCmd);cap.result=offset(result);
        std::memcpy(cap.command,storage,sizeof(storage));std::memcpy(cap.stats,Text_UI3DStats,sizeof(Text_UI3DStats));
    }else{
        using namespace store_font;
        for(unsigned i=0;i<0x7c;i+=4)W(place,i,f());
        Font_Capture=seed&1;Font_CaptureX=f();Font_CaptureY=f();Font_CaptureOffsetX=f();Font_CaptureOffsetY=f();
        Font_CaptureWidth=next();Font_CaptureHeight=next();
        for(auto&v:cap.rect)v=f();
        int ha=int(next()%15)-2,va=int(next()%15)-2;
        if(b){
            W(b,0x27f1b60,Font_Capture);W(b,0x27f1b64,Font_CaptureX);W(b,0x27f1b68,Font_CaptureY);
            W(b,0x27f1b6c,Font_CaptureOffsetX);W(b,0x27f1b70,Font_CaptureOffsetY);
            W(b,0x2cc0658,Font_CaptureWidth);W(b,0x2cc065c,Font_CaptureHeight);
            using Fn=void(MS*)(const Byte*,float*,float*,float*,float*,int,int);
            reinterpret_cast<Fn>(b+0x1f65e0)(place,cap.rect,cap.rect+1,cap.rect+2,cap.rect+3,ha,va);
        }else ApplyRect(place,cap.rect,cap.rect+1,cap.rect+2,cap.rect+3,ha,va);
    }
    std::memcpy(output,&cap,sizeof(cap));
}
}
}
