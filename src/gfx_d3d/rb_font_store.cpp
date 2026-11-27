// Store MP original backend text cursor and icon-size selection.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
#include <xmmintrin.h>
#include "gfx_d3d/r_font.h"
namespace store_font_backend {
using Byte=unsigned char;using U32=std::uint32_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C" {
extern int Backend_TextTime; // 142CB7DD8
extern int Backend_IconTableCount; // 1428386C0
extern const Byte *Backend_IconTables[]; // 1428386A0
extern Byte Backend_ColorShuffle[16]; // 143012E70
void Backend_Quad(void*,float,float,float,float,float,float,float,float,float,float,float,U32); // 14072BFE0
}
// 140732260: cursor blink, glyph lookup, white RGB override for special glyphs.
void DrawCursor(void*material,Byte letter,float x,float y,float sinAngle,float cosAngle,
                const Byte*font,float sx,float sy,U32 color){
    if((Backend_TextTime/256)&1)return;
    const Byte*g=reinterpret_cast<const Byte*>(R_GetCharacterGlyph(reinterpret_cast<Font_s*>(const_cast<Byte*>(font)),letter));
    if(Byte(letter-1)<=5||Byte(letter-14)<=11||Byte(letter-28)<=3||Byte(letter+0x44)<=1)
        color=(color&0xff000000u)|0xffffffu;
    Backend_Quad(material,x,y+float(static_cast<signed char>(g[3]))*sy,1.0f,
                 float(g[5])*sx,float(g[6])*sy,R<float>(g,8),R<float>(g,12),
                 R<float>(g,16),R<float>(g,20),sinAngle,cosAngle,color);
}
// 140723340. Find the first equal signed hash, then the first adequate
// height among duplicates; if none is adequate, return the first entry.
// Like the original, the table is assumed to have a readable terminal guard.
const Byte* LookupIcon(int hash,int height){
    for(int i=0;i<Backend_IconTableCount;++i){
        const Byte*table=Backend_IconTables[i];
        int lo=0,hi=R<int>(table,8)-1;
        const Byte*entries=R<const Byte*>(table,16);
        while(lo<=hi){
            const int mid=(lo+hi)/2;
            const Byte*e=entries+mid*40;
            const int v=R<int>(e,8);
            if(mid>0&&R<int>(e-40,8)>=hash){hi=mid-1;continue;}
            if(v==hash){
                const Byte*first=e;
                while(R<int>(e,24)<height){
                    e+=40;
                    if(R<int>(e,8)!=hash)return first;
                }
                return e;
            }
            if(v<hash)lo=mid+1;else hi=mid-1;
        }
    }
    return nullptr;
}
// 1400F2040: SSE floor(x*255+0.5), saturation, then original
// runtime channel shuffle. Preserve SSE NaN and signed-zero semantics.
void ConvertColor(const float*color,Byte*out){
    const __m128 x=_mm_add_ps(_mm_mul_ps(_mm_loadu_ps(color),_mm_set1_ps(255.0f)),_mm_set1_ps(0.5f));
    const __m128i absMask=_mm_set1_epi32(0x7fffffff);
    const __m128 mask=_mm_castsi128_ps(_mm_cmpgt_epi32(_mm_set1_epi32(0x4b000000),_mm_and_si128(_mm_castps_si128(x),absMask)));
    const __m128 trunc=_mm_cvtepi32_ps(_mm_cvttps_epi32(x));
    const __m128 floor=_mm_add_ps(trunc,_mm_cvtepi32_ps(_mm_castps_si128(_mm_cmplt_ps(x,trunc))));
    const __m128 rounded=_mm_or_ps(_mm_and_ps(floor,mask),_mm_andnot_ps(mask,x));
    const __m128 clamped=_mm_max_ps(_mm_setzero_ps(),_mm_min_ps(rounded,_mm_set1_ps(255.0f)));
    int lanes[4];_mm_storeu_si128(reinterpret_cast<__m128i*>(lanes),_mm_cvttps_epi32(_mm_max_ps(clamped,_mm_setzero_ps())));
    const auto*bytes=reinterpret_cast<const Byte*>(lanes);
    for(unsigned i=0;i<4;++i)out[i]=bytes[Backend_ColorShuffle[3-i]];
}
}
#endif
