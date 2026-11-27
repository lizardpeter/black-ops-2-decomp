#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/r_font_aux_store.cpp"
#undef T6_SOURCE
#include "../src/gfx_d3d/r_font_state_store.cpp"
#include <cctype>
#include <cstdlib>
#define MS __attribute__((ms_abi))
const Glyph*R_GetCharacterGlyph(Font_s*f,unsigned int ch){
    return f->glyphs+((ch-32u)<=95?ch-32u:14);
}
namespace store_font {void ApplyRect(const unsigned char*,float*,float*,float*,float*,int,int){}}
namespace store_font_aux{
extern "C"{
void*Aux_White;
int Aux_IsSpace(int ch){return std::isspace(ch);}
void Aux_ResolveText(int client,const char*key,char*out){
    int i=0;out[i++]=char('0'+(client&7));
    while(*key)out[i++]=*key++;
    out[i]=0;
}
void Aux_Quad(const float*,float,const float*,void*){}
}
}
namespace store_font_state{
extern "C"{
const Byte*State_ViewportScale;
double State_Atof(const char*s){return std::atof(s);}
void State_SetVertexBuffers(void*,int,int,const void*,const void*,const void*){}
}
}
namespace {
using Byte=unsigned char;using U32=std::uint32_t;
static Byte font[64],glyph[96*24],state[0x1c00],view[0x150],dvar[48];
static char text[1024],ring[512];
struct Capture{int result,index,length;char text[1024];Byte state[0x1c00];int viewport[4];};
static Capture cap;
template<class T>void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
static int MS Space(int c){return store_font_aux::Aux_IsSpace(c);}
static void MS Resolve(int c,const char*k,char*out){store_font_aux::Aux_ResolveText(c,k,out);}
static void* MS Copy(void*d,const void*s,std::size_t n){return std::memcpy(d,s,n);}
static char* MS Find(char*s,const char*t){return std::strstr(s,t);}
static double MS Atof(const char*s){return std::atof(s);}
}
extern "C"{
void* AuxMock(unsigned i){void*m[]={reinterpret_cast<void*>(Space),reinterpret_cast<void*>(Resolve),reinterpret_cast<void*>(Copy),reinterpret_cast<void*>(Find),reinterpret_cast<void*>(Atof)};return m[i];}
unsigned AuxSize(){return sizeof(Capture);}
void AuxRun(void*image,unsigned op,unsigned seed,void*out){
    cap={};auto*b=static_cast<Byte*>(image);
    unsigned st=seed+1;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%4096)-2048)*0.0625f;};
    W(font,0x10,96);W(font,0x28,static_cast<const Byte*>(glyph));W(font,8,int(next()%100+1));
    for(unsigned i=0;i<96;++i){W(glyph,i*24,std::uint16_t(i+32));glyph[i*24+4]=Byte(next()>>24);}
    if(op<3){
        const char*parts[]={"a","XYZ","^1","^F","^^","^X","\n","\r","\t","\xc2\x80","\xe1\x80\x80"," ","Z","^H!!\x01x"};
        std::memset(text,0,sizeof(text));unsigned n=next()%30,pos=0;
        for(unsigned i=0;i<n;++i){auto*s=parts[next()%14];unsigned l=unsigned(std::strlen(s));std::memcpy(text+pos,s,l);pos+=l;}
        int max=int(next()%100)-5,width=int(next()%2500)-5;float scale=f()*0.03125f;
        if(op==0){
            const char*ret;
            if(b){using F=const char*(MS*)(const char*,int,int,const Byte*,float);ret=reinterpret_cast<F>(b+0x71a9c0)(text,max,width,font,scale);}
            else ret=store_font_aux::WrapPosition(text,max,width,font,scale);
            cap.result=int(ret-text);
        }else if(op==1){
            // Complete, valid byte span in circular storage. No partial icon tokens.
            int start=int(next()%128);std::memset(ring,0,sizeof(ring));std::memcpy(ring+start,text,pos);
            if(b){using F=int(MS*)(const Byte*,int,int,int,const Byte*);cap.result=reinterpret_cast<F>(b+0x71abe0)(reinterpret_cast<Byte*>(ring),512,start,pos,font);}
            else cap.result=store_font_aux::RingTextWidth(reinterpret_cast<Byte*>(ring),512,start,pos,font);
        }else{
            const char*cases[]={"a [{ONE}] b [{TWO}]","[{a}]", "no key","[{}]","x [{unterminated","abc [{x}] tail","[{k}][{j}]"};
            std::strcpy(text,cases[next()%7]);cap.length=int(std::strlen(text));cap.index=int(next()%unsigned(cap.length+1));int client=int(next()%8);
            if(b){using F=bool(MS*)(int,int*,int*,char*);cap.result=reinterpret_cast<F>(b+0xf2a90)(client,&cap.index,&cap.length,text);}
            else cap.result=store_font_aux::ReplaceText(client,&cap.index,&cap.length,text);
            std::memcpy(cap.text,text,1024);
        }
    }else{
        std::memset(state,0,sizeof(state));for(unsigned i=0;i<sizeof(view);i+=4)W(view,i,f());
        W(state,0x1b10,int(next()%3));state[0x1b50]=Byte(next()%3);W(state,0x1b54,int(next()%4000+1));W(state,0x1b58,int(next()%3000+1));
        for(unsigned i=0;i<4;++i){W(state,0x1b28+i*4,int(next()%4000)-100);cap.viewport[i]=int(next()%4000)-100;}
        W(state,0x1ae0,static_cast<const Byte*>(view));W(dvar,0x18,2);W(dvar,0x20,f()*0.03125f);store_font_state::State_ViewportScale=(seed&1)?dvar:nullptr;
        if(op==3){
            if(b){W(b,0x27e7018,store_font_state::State_ViewportScale);using F=void(MS*)(const Byte*,int*);reinterpret_cast<F>(b+0x766cc0)(state,cap.viewport);}
            else store_font_state::GetViewport(state,cap.viewport);
        }else if(op==4){
            if(b){using F=void(MS*)(Byte*,const int*);reinterpret_cast<F>(b+0x766dd0)(state,cap.viewport);}
            else store_font_state::SetViewportConstants(state,cap.viewport);
        }else{
            if(b){using F=void(MS*)(Byte*);reinterpret_cast<F>(b+0x764fc0)(state);}
            else store_font_state::SetWorldTextTransform(state);
        }
        std::memcpy(cap.state,state,sizeof(state));
    }
    std::memcpy(out,&cap,sizeof(cap));
}
}
