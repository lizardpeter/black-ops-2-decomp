// Store MP GPU material/target state bodies recovered from renderer closure.
#define T6_SOURCE "gfx_d3d/r_state.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_gpu{void SetCodeConstant(unsigned char**,const unsigned char*);}
namespace store_renderer_state{
using Byte=unsigned char;using U16=std::uint16_t;using U32=std::uint32_t;using U64=std::uint64_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
extern "C"{
void State_Blend(void*,void*,const float*,U32);
void State_DepthStencil(void*,void*,U32);
void State_Rasterizer(void*,void*);
void State_Texture(void*,unsigned,void*);
void State_Sampler(void*,unsigned,unsigned);
void State_Target(void*,unsigned,void*const*,void*);
void State_Viewport(void*,unsigned,const float*);
extern Byte State_Targets[][32];
extern unsigned State_TextureCount;
extern const char*State_CodeTextureNames[];
extern void*State_OverrideTexture;
void State_MissingTexture(unsigned,const char*,const char*,const char*,Byte);
}
// 14072B330: update three original cached material state objects.
void ApplyMaterialState(Byte*s,const Byte*m){
    void*ctx=R<void*>(s,0xd0);
    if(R<void*>(s,0x53b8)!=R<void*>(m,8)){State_Blend(ctx,R<void*>(m,8),nullptr,0xffffffff);W(s,0x53b8,R<void*>(m,8));}
    if(R<void*>(s,0x53c0)!=R<void*>(m,0x10)){State_DepthStencil(ctx,R<void*>(m,0x10),0);W(s,0x53c0,R<void*>(m,0x10));}
    if(R<void*>(s,0x53c8)!=R<void*>(m,0x18)){State_Rasterizer(ctx,R<void*>(m,0x18));W(s,0x53c8,R<void*>(m,0x18));}
}
// 140766F00: remove SRV alias hazards, bind target and reset viewport cache.
void SetRenderTarget(Byte**pair,Byte id){
    Byte*source=pair[0],*s=pair[1];Byte old=s[0x5404];
    if(id==old)return;
    void*texture=R<void*>(State_Targets[id],0);
    void*ctx=R<void*>(s,0xd0);
    if(texture)for(unsigned i=0;i<State_TextureCount;++i)if(R<void*>(s,0x50+i*8)==texture){
        W(s,0x50+i*8,static_cast<void*>(nullptr));State_Texture(ctx,i,nullptr);
    }
    old=s[0x5404];
    Byte*target=State_Targets[id];Byte*prev=State_Targets[old];
    void*rt=R<void*>(target,8);void*ds=R<void*>(target,0x10);
    if(R<void*>(prev,8)!=rt||R<void*>(prev,0x10)!=ds){
        if(R<void*>(prev,8)!=rt){
            W(s,0x53e0,U64(0));W(s,0x53e8,U32(R<U16>(target,0x18)));W(s,0x53ec,U32(R<U16>(target,0x1a)));
            W(s,0x11e4,U32(-1));W(s,0x11e8,U64(0x3f800000));
        }
        State_Target(ctx,1,reinterpret_cast<void*const*>(target+8),ds);
        float vp[6]={float(R<int>(s,0x53e0)),float(R<int>(s,0x53e4)),float(R<int>(s,0x53e8)),float(R<int>(s,0x53ec)),R<float>(s,0x11ec),R<float>(s,0x11e8)};
        State_Viewport(ctx,1,vp);
    }
    s[0x5404]=id;W(source,0x1b10,0);source[0x1b5c]=1;
}
static void BindTextureAndSampler(Byte*s,Byte tex,Byte sam,void*texture,Byte sampler){
    void*ctx=R<void*>(s,0xd0);
    if(R<void*>(s,0x50+tex*8)!=texture){W(s,0x50+tex*8,texture);State_Texture(ctx,tex,texture);}
    if(s[sam]!=sampler){
        s[sam]=sampler;
        if(R<U32>(s,0x10+sam*4)!=sampler){State_Sampler(ctx,sam,sampler);W(s,0x10+sam*4,U32(sampler));}
    }
}
static void WriteConstant(Byte*s,const Byte*arg,const void*data){
    unsigned slot=R<U16>(arg,6),off=R<U16>(arg,2),bytes=R<U16>(arg,4);
    W(R<Byte*>(s,0x32b0+slot*8),(off>>2)*8,U64(-1));
    std::memcpy(R<Byte*>(s,0x1190+slot*8)+off,data,bytes);s[0x11b0+slot]=1;
}
// 140774090: per-pass material literal, texture and code argument sequence.
void ApplyPassArgs(Byte**pair,int count,const Byte*arg){
    Byte*s=pair[1];Byte*source=pair[0];
    const Byte*material=R<const Byte*>(s,0x11c0);
    const Byte*constant=R<const Byte*>(material,0x70);
    if(constant)while(count&&R<U16>(arg,0)==0){
        while(R<U32>(constant,0)!=R<U32>(arg,8))constant+=32;
        WriteConstant(pair[1],arg,constant+16);arg+=16;--count;
    }
    while(count&&R<U16>(arg,0)==1){
        WriteConstant(pair[1],arg,R<const void*>(arg,8));arg+=16;--count;
    }
    const Byte*texture=R<const Byte*>(material,0x68);
    while(count&&R<U16>(arg,0)==2){
        while(R<U32>(texture,0)!=R<U32>(arg,8))texture+=24;
        void*t=R<void*>(texture,16);Byte sampler=texture[6];
        if(texture[8]&&R<int>(R<const Byte*>(source,0x1720),0x4c005c)!=0&&(R<U32>(R<const Byte*>(pair[1],0x11c0),0x1c)&0xe0000000)>0x20000000)t=State_OverrideTexture;
        BindTextureAndSampler(pair[1],arg[2],arg[3],t,sampler);arg+=16;--count;
    }
    while(count&&R<U16>(arg,0)==3){store_font_gpu::SetCodeConstant(pair,arg);arg+=16;--count;}
    while(count&&R<U16>(arg,0)==4){
        U32 i=R<U32>(arg,8);void*t=R<void*>(source,0x1530+i*8);Byte sampler=source[0x16e8+i];
        s=pair[1];
        if(!t)State_MissingTexture(i,State_CodeTextureNames[i]?State_CodeTextureNames[i]:"noname",R<const char*>(R<const Byte*>(s,0x11c0),0),R<const char*>(R<const Byte*>(s,0x11d0),0),s[0x11c8]);
        BindTextureAndSampler(pair[1],arg[2],arg[3],t,sampler);arg+=16;--count;
    }
}
}
#endif
