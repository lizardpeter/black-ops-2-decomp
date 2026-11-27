#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/rb_font_store.cpp"
#include <cassert>
#define MS __attribute__((ms_abi))
const Glyph* R_GetCharacterGlyph(Font_s*font,unsigned int letter){
    return font->glyphs+((letter-32u)<=95u?letter-32u:14u);
}
namespace store_font_backend {
using U=std::uint64_t;
static Byte glyphs[96*24],font[64],tables[2][24],entries[2][13*40];
struct Capture{U called;float args[11];U color,material,result;Byte bytes[4];};static Capture cap;
extern "C" {
int Backend_TextTime,Backend_IconTableCount;
const Byte*Backend_IconTables[2];
Byte Backend_ColorShuffle[16];
void Backend_Quad(void*m,float x,float y,float z,float w,float h,float s0,float t0,float s1,float t1,float sn,float cs,U32 color){
    ++cap.called;float a[]={x,y,z,w,h,s0,t0,s1,t1,sn,cs};std::memcpy(cap.args,a,sizeof(a));cap.color=color;cap.material=reinterpret_cast<U>(m);
}
}
static void MS MQuad(void*m,float x,float y,float z,float w,float h,float s0,float t0,float s1,float t1,float sn,float cs,U32 color){Backend_Quad(m,x,y,z,w,h,s0,t0,s1,t1,sn,cs,color);}
template<class T>static void W(void*p,unsigned o,T v){std::memcpy(static_cast<Byte*>(p)+o,&v,sizeof(v));}
extern "C" {
void* BackendMock(){return reinterpret_cast<void*>(MQuad);}
unsigned BackendSize(){return sizeof(Capture);}
void BackendRun(void*image,unsigned op,unsigned seed,void*out){
    cap={};auto*b=static_cast<Byte*>(image);unsigned state=seed+1;
    auto next=[&](){state=state*1664525u+1013904223u;return state;};
    auto f=[&](){return float(int(next()%2048)-1024)*0.125f;};
    if(op==0){
        W(font,0x28,static_cast<const Byte*>(glyphs));W(font,0x10,96);
        for(unsigned i=0;i<96;++i){
            W(glyphs,i*24,std::uint16_t(i+32));glyphs[i*24+3]=Byte(next());glyphs[i*24+5]=Byte(next());glyphs[i*24+6]=Byte(next());
            for(unsigned k=8;k<24;k+=4)W(glyphs,i*24+k,f());
        }
        Backend_TextTime=int(next());Byte ch=Byte(next());float x=f(),y=f(),sn=f(),cs=f(),sx=f(),sy=f();U32 color=next();void*m=reinterpret_cast<void*>(0x12345678);
        if(b){W(b,0x2cb7dd8,Backend_TextTime);using F=void(MS*)(void*,Byte,float,float,float,float,const Byte*,float,float,U32);reinterpret_cast<F>(b+0x732260)(m,ch,x,y,sn,cs,font,sx,sy,color);}
        else DrawCursor(m,ch,x,y,sn,cs,font,sx,sy,color);
    }else if(op==1){
        Backend_IconTableCount=int(next()%3);
        for(unsigned j=0;j<2;++j){
            Backend_IconTables[j]=tables[j];W(tables[j],8,12);W(tables[j],16,static_cast<const Byte*>(entries[j]));
            for(unsigned i=0;i<12;++i){W(entries[j],i*40+8,int(i/3)*4+int(j)*2-6);W(entries[j],i*40+24,int(i%3)*12+8);}
            W(entries[j],12*40+8,999);
        }
        int hash=int(next()%23)-11,height=int(next()%70)-10;
        const Byte*result;
        if(b){W(b,0x28386c0,Backend_IconTableCount);std::memcpy(b+0x28386a0,Backend_IconTables,sizeof(Backend_IconTables));using F=const Byte*(MS*)(int,int);result=reinterpret_cast<F>(b+0x723340)(hash,height);}
        else result=LookupIcon(hash,height);
        cap.result=0;
        if(result){bool found=false;for(unsigned j=0;j<2;++j)for(unsigned i=0;i<12;++i)if(result==entries[j]+i*40){cap.result=1+j*12+i;found=true;}assert(found);}
    }else{
        float color[4];for(float&v:color){U32 bits=next();if(seed&1)v=f()*0.015625f;else std::memcpy(&v,&bits,4);}
        for(unsigned i=0;i<16;++i)Backend_ColorShuffle[i]=Byte(next()%16);
        if(b){std::memcpy(b+0x3012e70,Backend_ColorShuffle,16);using F=void(MS*)(const float*,Byte*);reinterpret_cast<F>(b+0xf2040)(color,cap.bytes);}
        else ConvertColor(color,cap.bytes);
    }
    std::memcpy(out,&cap,sizeof(cap));
}
}
}
