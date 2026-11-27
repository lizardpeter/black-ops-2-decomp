// Store MP 140735340, complete original world-space text loop.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include "gfx_d3d/r_font.h"
namespace store_font_geom{
void GlyphQuad3D(const unsigned char*,const float*,const float*,const float*,const unsigned char*,std::uint32_t);
extern "C" {extern int Geom_IndexCount;void Geom_Flush();}
}
namespace store_font_state{void SetWorldTextTransform(unsigned char*);}
namespace store_font_world {
using Byte=unsigned char;using U32=std::uint32_t;
extern "C" {extern Byte*World_SourceState;}
void DrawTextInSpace(const Byte*text,const Byte*font,const float*origin,const float*right,const float*up,U32 color){
    const Byte*material;std::memcpy(&material,font+0x18,sizeof(material));
    if(store_font_geom::Geom_IndexCount)store_font_geom::Geom_Flush();
    store_font_state::SetWorldTextTransform(World_SourceState);
    float pos[3]={origin[0],origin[1],origin[2]};
    while(*text){
        U32 ch=*text++;
        if((ch&0xe0)==0xc0)ch=((ch&31)<<6)|(*text++&63);
        else if((ch&0xf0)==0xe0){ch=((ch&15)<<12)|((text[0]&63)<<6)|(text[1]&63);text+=2;}
        const Byte*g=reinterpret_cast<const Byte*>(R_GetCharacterGlyph(reinterpret_cast<Font_s*>(const_cast<Byte*>(font)),ch));
        U32 c=color;
        if(ch-1<6||ch-14<12||ch-28<4||ch-0xbc<2)c=(color&0xff000000u)|0xffffffu;
        float x0=float(static_cast<signed char>(g[2])),y0=float(static_cast<signed char>(g[3]));
        float w=float(g[5]),h=float(g[6]),advance=float(g[4]);
        float o[3],a[3],b[3];
        for(unsigned i=0;i<3;++i){o[i]=right[i]*x0+pos[i]+up[i]*y0;a[i]=right[i]*w;b[i]=up[i]*h;}
        store_font_geom::GlyphQuad3D(material,o,a,b,g,c);
        for(unsigned i=0;i<3;++i)pos[i]+=advance*right[i];
    }
    if(store_font_geom::Geom_IndexCount)store_font_geom::Geom_Flush();
}
}
#endif
