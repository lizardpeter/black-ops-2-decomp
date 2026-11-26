// Original Store MP text render command consumers, 1407356A0/140735840.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_text_loop {
using Byte=unsigned char;using U32=std::uint32_t;
void DrawTeleType(const Byte*,float,float,float,float,const Byte*,float,float,U32,U32,int,Byte,int,int,int);
void DrawText2D(const Byte*,float,float,float,const Byte*,float,float,float,float,U32,int,U32,int,Byte,float,U32,int,int,int,int,int,int,const Byte*,const Byte*);
}
namespace store_font_world {void DrawTextInSpace(const unsigned char*,const unsigned char*,const float*,const float*,const float*,std::uint32_t);}
namespace store_text_dispatch {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T>static T R(const Byte*p,unsigned n){T v;std::memcpy(&v,p+n,sizeof(v));return v;}
extern "C" std::uint64_t Text_SinCos(float); // original packed return: sin low lane, cos high lane.
void DrawText2DCmd(const Byte**exec){
    const Byte*p=*exec;
    const int type=R<int>(p,4);
    if(type==0){
        std::uint64_t packed=Text_SinCos(R<float>(p,0x18)*0.017453292f);
        float sc[2];std::memcpy(sc,&packed,8);
        store_text_loop::DrawText2D(p+0x74,R<float>(p,8),R<float>(p,0xc),R<float>(p,0x10),R<const Byte*>(p,0x20),R<float>(p,0x28),R<float>(p,0x2c),sc[0],sc[1],R<U32>(p,0x30),R<int>(p,0x34),R<U32>(p,0x38),R<int>(p,0x3c),p[0x40],R<float>(p,0x70),R<U32>(p,0x44),R<int>(p,0x48),R<int>(p,0x4c),R<int>(p,0x50),R<int>(p,0x54),R<int>(p,0x58),R<int>(p,0x5c),R<const Byte*>(p,0x60),R<const Byte*>(p,0x68));
    }else if(type==1){
        store_text_loop::DrawTeleType(p+0x74,R<float>(p,8),R<float>(p,0xc),R<float>(p,0x10),0,R<const Byte*>(p,0x20),R<float>(p,0x28),R<float>(p,0x2c),R<U32>(p,0x30),R<U32>(p,0x38),R<int>(p,0x3c),p[0x40],0,R<int>(p,0x48),R<int>(p,0x4c));
    }
    *exec+=R<std::uint16_t>(*exec,0);
}
void DrawText3DCmd(const Byte**exec){
    const Byte*p=*exec;
    store_font_world::DrawTextInSpace(p+0x34,R<const Byte*>(p,0x10),reinterpret_cast<const float*>(p+4),reinterpret_cast<const float*>(p+0x18),reinterpret_cast<const float*>(p+0x24),R<U32>(p,0x30));
    *exec+=R<std::uint16_t>(*exec,0);
}
}
#endif
