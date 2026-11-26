// Original Store MP viewport, world text transform and vertex buffer state.
#define T6_SOURCE "gfx_d3d/r_state.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
namespace store_font_state {
using Byte=unsigned char;using U16=std::uint16_t;using U32=std::uint32_t;using U64=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
static void Increment16(Byte*p,unsigned n){W(p,n,U16(R<U16>(p,n)+1));}
extern "C" {
extern const Byte*State_ViewportScale;
double State_Atof(const char*);
void State_SetVertexBuffers(void*,int,int,const void*,const void*,const void*);
}
static int Trunc(float f){return _mm_cvttss_si32(_mm_set_ss(f));}
// 140766CC0: original viewport selection and resolution scaling.
void GetViewport(const Byte*s,int*out){
    if(s[0x1b50]==1){out[0]=out[1]=0;out[2]=R<int>(s,0x1b54);out[3]=R<int>(s,0x1b58);}
    else std::memcpy(out,s+0x1b28,16);
    float scale=0;
    if(State_ViewportScale){
        int t=R<int>(State_ViewportScale,0x18);
        scale=t==2?R<float>(State_ViewportScale,0x20):t==6?float(R<int>(State_ViewportScale,0x20)):float(State_Atof(R<const char*>(State_ViewportScale,0x20)));
    }
    if(R<int>(s,0x1b10)!=2&&(scale<1||scale>1)){
        out[0]+=Trunc(((scale-1)*float(out[2]))*(-0.5f));
        out[1]+=Trunc((float(out[3])*(scale-1))*(-0.5f));
        int w=Trunc(float(out[2])*scale),h=Trunc(float(out[3])*scale);
        out[2]=w>1?w:1;out[3]=h>1?h:1;
    }
}
// 140766DD0: viewport-derived shader constants and version increments.
void SetViewportConstants(Byte*s,const int*vp){
    s[0x1b5c]=0;
    const float width=float(R<int>(s,0x1b54)),height=float(R<int>(s,0x1b58));
    const float iw=1.0f/width,ih=1.0f/height;
    W(s,0x910,width);W(s,0x914,height);W(s,0x918,iw);W(s,0x91c,ih);Increment16(s,0x18e2);
    const float x=float(vp[2])*iw*0.5f,y=float(vp[3])*ih;
    W(s,0xbe0,x);W(s,0xbe4,y*(-0.5f));W(s,0xbe8,0);W(s,0xbec,1.0f);Increment16(s,0x193c);
    W(s,0xbf0,float(vp[0])*iw+x);W(s,0xbf4,float(vp[1])*ih+y*0.5f);W(s,0xbf8,U64(0));Increment16(s,0x193e);
}
// 140764FC0: switch to world text transform, copy the source view parameters.
void SetWorldTextTransform(Byte*s){
    if(R<int>(s,0x1b10)==1)return;
    W(s,0x1b10,1);std::memcpy(s+0x1730,R<const Byte*>(s,0x1ae0),0x150);
    if(R<float>(s,0x183c)==0){W(s,0x1ac0,U64(0));W(s,0x1ac8,0);}
    else{W(s,0x1ac0,R<U32>(s,0x1830));W(s,0x1ac4,R<U32>(s,0x1834));W(s,0x1ac8,R<U32>(s,0x1838));}
    W(s,0x1acc,1.0f);
    const unsigned offsets[]={0x1aa6,0x1aa8,0x1aaa,0x1aae,0x1aac,0x1ab0,0x1ab4};
    for(unsigned o:offsets)Increment16(s,o);
    W(s,0x1a66,R<U16>(s,0x1aa6));
    const float m[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    std::memcpy(s,m,64);
    W(s,0x30,0.0f-R<float>(s,0x1ac0));W(s,0x34,0.0f-R<float>(s,0x1ac4));W(s,0x38,0.0f-R<float>(s,0x1ac8));
}
// 14073D540: update cached vertex buffer streams and only issue changed interval.
void SetVertexBuffer(Byte*s,const void*buffer,int offset,int stride){
    int first=-1,last=-1;
    if(R<const void*>(s,0x28)!=buffer||R<int>(s,0x40)!=offset||R<int>(s,0x18)!=stride){
        W(s,0x28,buffer);W(s,0x40,offset);W(s,0x18,stride);first=last=0;
    }
    for(int i=1;i<3;++i){
        if(R<const void*>(s,0x28+8*i)||R<int>(s,0x40+4*i)||R<int>(s,0x18+4*i)){
            W(s,0x28+8*i,U64(0));W(s,0x40+4*i,0);W(s,0x18+4*i,0);
            if(first<0)first=i;
            last=i;
        }
    }
    if(first>=0)State_SetVertexBuffers(R<void*>(s,0),first,last-first+1,s+0x28+first*8,s+0x18+first*4,s+0x40+first*4);
}
}
#endif
