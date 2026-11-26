// Store MP glyph geometry and material batching, based on original PE bodies.
// No GPU API or font format changes.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_geom {
using Byte=unsigned char;using U32=std::uint32_t;using U16=std::uint16_t;
struct Vertex{float x,y,z,w;U32 color;float s,t;U32 normal;};
static_assert(sizeof(Vertex)==32,"original tess vertex layout");
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C" {
extern Vertex*Geom_Vertices; // 14283E660
extern U16*Geom_Indices; // 142A3E620
extern int Geom_IndexCount,Geom_VertexCount; // 142C3E628,142C3E62C
extern std::uint64_t Geom_Stage;
extern int Geom_VertexDecl; // 142C3E630,142C3E63C
extern int Geom_PrimState; // 142C41500
extern const Byte*Geom_Material; // 142C425E0
extern Byte Geom_TechniqueType; // 142C425E8
extern void*Geom_Technique; // 142C425F0
void Geom_Flush(); // 14073D680
}
static void RefreshTechnique(){
    Geom_Technique=R<void*>(R<const Byte*>(Geom_Material,0x60),0x10+8*Geom_TechniqueType);
}
// 14073D610: original material/technique switch with conditional batch flush.
void BeginSurface(const Byte*material,Byte type){
    if(Geom_Material==material&&Geom_TechniqueType==type)return;
    if(Geom_IndexCount)Geom_Flush();
    Geom_VertexDecl=4;Geom_Stage=0;Geom_PrimState=0;
    Geom_Technique=R<void*>(R<const Byte*>(material,0x60),0x10+8*type);
    Geom_Material=material;Geom_TechniqueType=type;
}
static unsigned AllocateQuad(const Byte*material,bool truncateIndex){
    BeginSurface(material,2);
    if(Geom_VertexCount+4>0xfffe||Geom_IndexCount+6>0x100000){
        Geom_Flush();Geom_Stage=0;Geom_PrimState=0;RefreshTechnique();
    }
    const unsigned base=static_cast<unsigned>(Geom_VertexCount);
    auto*i=Geom_Indices+Geom_IndexCount;
    i[0]=U16(base+3);i[1]=U16(base);i[2]=U16(base+2);i[3]=U16(base+2);i[4]=U16(base);i[5]=U16(base+1);
    Geom_IndexCount+=6;Geom_VertexCount+=4;
    return truncateIndex?base&0xffff:base;
}
static Vertex V(float x,float y,float z,float w,U32 color,float s,float t){return {x,y,z,w,color,s,t,0x3ffe7f7f};}
// 14072B8F0: axis-aligned textured rectangle.
void StretchPic(const Byte*m,float x,float y,float w,float h,float s0,float t0,float s1,float t1,U32 c){
    auto*v=Geom_Vertices+AllocateQuad(m,false);const float r=x+w,b=y+h;
    v[0]=V(x,y,0,1,c,s0,t0);v[1]=V(r,y,0,1,c,s1,t0);
    v[2]=V(r,b,0,1,c,s1,t1);v[3]=V(x,b,0,1,c,s0,t1);
}
// 14072BFE0: rotated glyph quad. Keep the original rotation around the first vertex.
void StretchPicRotate(const Byte*m,float x,float y,float z,float w,float h,
                      float s0,float t0,float s1,float t1,float sn,float cs,U32 c){
    auto*v=Geom_Vertices+AllocateQuad(m,true);
    const float rx=w*cs+x,ry=w*sn+y,dx=-(h*sn),dy=h*cs;
    v[0]=V(x,y,0,z,c,s0,t0);v[1]=V(rx,ry,0,z,c,s1,t0);
    v[2]=V(rx+dx,ry+dy,0,z,c,s1,t1);v[3]=V(dx+x,dy+y,0,z,c,s0,t1);
}
// 140735080: a glyph in world space, expressed as origin + two edge vectors.
void GlyphQuad3D(const Byte*m,const float*o,const float*a,const float*b,const Byte*g,U32 c){
    auto*v=Geom_Vertices+AllocateQuad(m,true);
    const float s0=R<float>(g,8),t0=R<float>(g,12),s1=R<float>(g,16),t1=R<float>(g,20);
    v[0]=V(o[0],o[1],o[2],1,c,s0,t0);
    v[1]=V(o[0]+a[0],o[1]+a[1],o[2]+a[2],1,c,s1,t0);
    v[2]=V(o[0]+a[0]+b[0],o[1]+a[1]+b[1],o[2]+a[2]+b[2],1,c,s1,t1);
    v[3]=V(o[0]+b[0],o[1]+b[1],o[2]+b[2],1,c,s0,t1);
}
}
#endif
