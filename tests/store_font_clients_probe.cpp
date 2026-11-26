#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/cgame/cg_hud_clients_store.cpp"
#undef T6_SOURCE
#include "../src/gfx_d3d/r_static_tables_store.cpp"
#include <cstdlib>
#include <cstdio>
#define MS __attribute__((ms_abi))
using B=unsigned char;using U=std::uint32_t;
template<class T>static void W(void*p,unsigned off,T v){std::memcpy(static_cast<B*>(p)+off,&v,sizeof v);}
static B state[0x73000],spectator[0x1b4],dvars[8][48],relation[8],chat[0x1128*2],placement[0x7c],font[16],entity[504];
static B tables[400],sizes[200];
struct Capture{int result,client,editX,editY,widthMax,cmdMax,style,cursor,ha,va;U formatHash,translateHash,color[4];float scale,rect[4],draw[8];B ptrs[400],lengths[200];unsigned ready;};
static Capture*cap;static char label[64];static int width;
static U Hash(const char*s){U h=2166136261u;for(;*s;++s)h=(h^B(*s))*16777619u;return h;}
namespace store_hud_clients{
extern "C"{
const B *Clients_Count,*Clients_Enable,*Clients_State=state,*Clients_Color[6],*Clients_Relation=relation,*Clients_Placement=placement;
B*Clients_Chat=chat;U Clients_Mode,Clients_Flags,Clients_ChatFlags;
int Clients_Capture;float Clients_CaptureY,Clients_HudScale;
int Clients_Atoi(const char*s){return std::atoi(s);}
void Clients_ParseColor(const char*s,U*out){*out=Hash(s);}
const char* Clients_Format(const char*f,const char*s){cap->formatHash=Hash(f);std::snprintf(label,sizeof label,f,s);return label;}
int Clients_Width(int,const char*s,int n,const void*){cap->widthMax=n;cap->result=int(Hash(s));return width;}
void Clients_Edit(int c,B*,int x,int y){cap->client=c;cap->editX=x;cap->editY=y;}
}
}
namespace store_loc_format{const char* TranslateExe(const char*s){cap->translateHash=Hash(s);return s;}}
namespace store_font_core{void*DefaultFont(float f){cap->scale=f;return font;}}
namespace store_font{void ApplyRect(const B*,float*x,float*y,float*sx,float*sy,int ha,int va){cap->ha=ha;cap->va=va;cap->rect[0]=*x;cap->rect[1]=*y;cap->rect[2]=*sx;cap->rect[3]=*sy;*x+=3;*y-=5;*sx*=2;*sy*=4;}}
namespace store_text_cmd{B*AddBaseDrawTextCmd(const char*s,int n,const void*,float x,float y,float z,float sx,float sy,float angle,const float*c,int style,int cursor,B,float pad){cap->cmdMax=n;cap->style=style;cap->cursor=cursor;cap->draw[0]=x;cap->draw[1]=y;cap->draw[2]=z;cap->draw[3]=sx;cap->draw[4]=sy;cap->draw[5]=angle;cap->draw[6]=pad;cap->draw[7]=float(Hash(s));std::memcpy(cap->color,c,16);return nullptr;}}
namespace store_static_tables{extern "C"{B*Static_BufferBase;B*Static_Pointers=tables;B*Static_Sizes=sizes;B Static_Ready;}}
static int MS MAtoi(const char*s){return store_hud_clients::Clients_Atoi(s);}
static void MS MColor(const char*s,U*c){store_hud_clients::Clients_ParseColor(s,c);}
static const char*MS MTranslate(const char*s){return store_loc_format::TranslateExe(s);}
static const char*MS MFormat(const char*f,const char*s){return store_hud_clients::Clients_Format(f,s);}
static void*MS MFont(float f){return store_font_core::DefaultFont(f);}
static void MS MRect(const B*p,float*x,float*y,float*sx,float*sy,int ha,int va){store_font::ApplyRect(p,x,y,sx,sy,ha,va);}
static B*MS MDraw(const char*s,int n,const void*f,float x,float y,float z,float sx,float sy,float a,const float*c,int style,int cursor,B v,float p){return store_text_cmd::AddBaseDrawTextCmd(s,n,f,x,y,z,sx,sy,a,c,style,cursor,v,p);}
static int MS MWidth(int c,const char*s,int n,const void*f){return store_hud_clients::Clients_Width(c,s,n,f);}
static void MS MEdit(int c,B*p,int x,int y){store_hud_clients::Clients_Edit(c,p,x,y);}
extern "C"{
void*ClientsMock(unsigned i){void*ptrs[]={reinterpret_cast<void*>(MAtoi),reinterpret_cast<void*>(MColor),reinterpret_cast<void*>(MTranslate),reinterpret_cast<void*>(MFormat),reinterpret_cast<void*>(MFont),reinterpret_cast<void*>(MRect),reinterpret_cast<void*>(MDraw),reinterpret_cast<void*>(MWidth),reinterpret_cast<void*>(MEdit)};return ptrs[i];}
unsigned ClientsSize(){return sizeof(Capture);}
void ClientsRun(void*image,unsigned op,unsigned seed,void*out){
    using namespace store_hud_clients;using namespace store_static_tables;
    auto*b=static_cast<B*>(image);Capture c{};cap=&c;
    U s=seed;auto next=[&](){s=s*1664525u+1013904223u;return s;};
    auto dv=[&](unsigned i,unsigned t,int v)->const B*{
        B*d=dvars[i];std::memset(d,0,48);if(!t)return nullptr;W(d,0x18,t==3?6:t==4?7:t==5?8:t);
        if(t==1)W(d,0x20,B(v!=0));else if(t==2)W(d,0x20,float(v)+0.25f);else if(t==5)W(d,0x20,v?"5":"0");else W(d,0x20,v);return d;
    };
    Clients_Enable=dv(0,seed%6,int(next()%7)-3);
    Clients_Count=dv(1,seed%6,int(next()%9)-2);
    for(unsigned i=0;i<6;++i){B*d=dvars[i+2];std::memset(d,0,48);W(d,0x18,seed&(1u<<(8+i))?9:8);if(R<int>(d,0x18)==9)W(d,0x20,next());else W(d,0x20,i&1?"color-a":"color-b");Clients_Color[i]=d;}
    int self=int(next()%8),target=int(next()%8);
    W(state,0,self);W(state,0x30,seed&1?spectator:nullptr);W(spectator,0x1b0,int(next()%8));state[0x6a3b4]=B(next()%2);
    Clients_Mode=next()%4;Clients_Flags=next()%16;
    for(unsigned i=0;i<8;++i){unsigned k=0x6bfa0+i*0x870;int ts[]={0,1,2,9};W(state,0x6b9b4+i*0x870,ts[next()%4]);W(state,k-0x618,int(next()%3));W(state,k-4,int(next()%5));W(state,k,int(next()%2));relation[i]=B(next()%2);}
    W(entity,500,int(next()%5));float color[]={0,0,0,float(next()%128)/128.0f};
    int client=int(next()%3)-1;Clients_ChatFlags=next()%64;
    Clients_Capture=int(next()%2);Clients_CaptureY=float(next()%8)*0.25f;Clients_HudScale=float(next()%8+1)*0.25f;
    W(placement,4,float(next()%8+1)*0.5f);W(font,8,int(next()%64+1));width=int(next()%600);
    for(unsigned i=0;i<2;++i){W(chat,i*0x1128+0x10,float(next()%60+1));W(chat,i*0x1128+0x118,int(next()%2));}
    if(b){
        W(b,0x1f04908,Clients_Enable);W(b,0x1f04878,Clients_Count);W(b,0xe53a20,Clients_State);
        unsigned col[]={0xe53b58,0xe53b10,0xe53b48,0xe53b50,0xe53af8,0xe53b38};for(unsigned i=0;i<6;++i)W(b,col[i],Clients_Color[i]);
        W(b,0x25a3308,Clients_Mode);W(b,0x25a330c,B(Clients_Flags));std::memcpy(b+0x2cc0a34,relation,8);
        W(b,0xe61794,Clients_ChatFlags);W(b,0x27f1b60,Clients_Capture);W(b,0x27f1b68,Clients_CaptureY);W(b,0xe6287c,Clients_HudScale);std::memcpy(b+0xe63280,placement,sizeof placement);std::memcpy(b+0xe5d240,chat,sizeof chat);
        if(op==0)c.result=reinterpret_cast<bool(MS*)()>(b+0xa9960)();
        else if(op==1)c.result=reinterpret_cast<int(MS*)(const void*,const B*)>(b+0xea250)(nullptr,entity);
        else if(op==2)reinterpret_cast<void(MS*)(int,float*)>(b+0xde7b0)(target,color);
        else if(op==3)reinterpret_cast<void(MS*)(int)>(b+0xd2ff0)(client);
        else{reinterpret_cast<void(MS*)()>(b+0x70c5a0)();std::memcpy(c.ptrs,b+0x274d3a0,400);std::memcpy(c.lengths,b+0x274d210,200);c.ready=b[0x274bbfc];}
    }else{
        if(op==0)c.result=HudEnabled();
        else if(op==1)c.result=FindHudClient(nullptr,entity);
        else if(op==2)ClientColor(target,color);
        else if(op==3)DrawChatPrompt(client);
        else{InitializeWorkTables();std::memcpy(c.ptrs,tables,400);std::memcpy(c.lengths,sizes,200);c.ready=Static_Ready;}
    }
    if(op==2)std::memcpy(c.color,color,16);
    std::memcpy(out,&c,sizeof c);
}
void StaticBase(void*base){store_static_tables::Static_BufferBase=static_cast<B*>(base)+0x266d500;}
}
