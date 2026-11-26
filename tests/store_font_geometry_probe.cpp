#define T6_STORE
#define T6_STORE_FONT_REPLAY
#include "../src/gfx_d3d/rb_font_geometry_store.cpp"
#define MS __attribute__((ms_abi))
namespace store_font_geom{
static Vertex vertices[65536];static U16 indices[0x100000];static Byte mats[2][112],tech[2][80];
static Byte*base;
struct Capture{int flushes,indexCount,vertexCount,stage,decl,prim,techType;int spare;std::uint64_t material,technique;Vertex verts[4];U16 idx[6];U16 padding[2];};
static Capture cap;
extern "C"{
Vertex*Geom_Vertices=vertices;U16*Geom_Indices=indices;
int Geom_IndexCount,Geom_VertexCount;std::uint64_t Geom_Stage;int Geom_VertexDecl,Geom_PrimState;
const Byte*Geom_Material;Byte Geom_TechniqueType;void*Geom_Technique;
void Geom_Flush(){++cap.flushes;Geom_IndexCount=Geom_VertexCount=0;}
}
template<class T>static void W(void*p,unsigned n,T v){std::memcpy(static_cast<Byte*>(p)+n,&v,sizeof(v));}
static void MS FlushOriginal(){++cap.flushes;W(base,0x2c3e628,std::uint64_t(0));}
extern "C"{
void* GeometryMock(){return reinterpret_cast<void*>(FlushOriginal);}
unsigned GeometrySize(){return sizeof(Capture);}
void GeometryRun(void*image,unsigned op,unsigned seed,void*out){
    cap={};base=static_cast<Byte*>(image);unsigned st=seed+1;auto next=[&](){st=st*1664525u+1013904223u;return st;};
    auto f=[&](){return float(int(next()%4096)-2048)*0.0625f;};
    for(unsigned i=0;i<2;++i){W(mats[i],0x60,static_cast<const Byte*>(tech[i]));for(unsigned j=0;j<8;++j)W(tech[i],0x10+j*8,reinterpret_cast<void*>(0x4000+16*i+j));}
    auto*material=mats[next()&1];Geom_Material=mats[(seed>>3)&1];Geom_TechniqueType=Byte((seed>>4)%5);
    Geom_Technique=reinterpret_cast<void*>(0x12345678);Geom_Stage=0xaabbccdd12345678ull;Geom_VertexDecl=17;Geom_PrimState=23;
    Geom_IndexCount=(seed&1)?int(next()%32)*6:0;Geom_VertexCount=(seed&2)?int(next()%32)*4:0;
    if(seed&0x100)Geom_VertexCount=65532;
    if(seed&0x200)Geom_IndexCount=0x100000-3;
    float x=f(),y=f(),z=f(),w=f(),h=f(),s0=f(),t0=f(),s1=f(),t1=f(),sn=f(),cs=f();U32 c=next();
    float o[3]={x,y,z},a[3]={w,h,sn},b[3]={cs,s0,t1};Byte glyph[24]={};
    W(glyph,8,s0);W(glyph,12,t0);W(glyph,16,s1);W(glyph,20,t1);Byte type=Byte(next()%5);
    if(base){
        W(base,0x2c3e628,Geom_IndexCount);W(base,0x2c3e62c,Geom_VertexCount);W(base,0x2c3e630,Geom_Stage);W(base,0x2c3e63c,Geom_VertexDecl);W(base,0x2c41500,Geom_PrimState);
        W(base,0x2c425e0,Geom_Material);W(base,0x2c425e8,Geom_TechniqueType);W(base,0x2c425f0,Geom_Technique);
        if(op==0){using F=void(MS*)(const Byte*,Byte);reinterpret_cast<F>(base+0x73d610)(material,type);}
        if(op==1){using F=void(MS*)(const Byte*,float,float,float,float,float,float,float,float,U32);reinterpret_cast<F>(base+0x72b8f0)(material,x,y,w,h,s0,t0,s1,t1,c);}
        if(op==2){using F=void(MS*)(const Byte*,float,float,float,float,float,float,float,float,float,float,float,U32);reinterpret_cast<F>(base+0x72bfe0)(material,x,y,z,w,h,s0,t0,s1,t1,sn,cs,c);}
        if(op==3){using F=void(MS*)(const Byte*,const float*,const float*,const float*,const Byte*,U32);reinterpret_cast<F>(base+0x735080)(material,o,a,b,glyph,c);}
        Geom_IndexCount=R<int>(base,0x2c3e628);Geom_VertexCount=R<int>(base,0x2c3e62c);Geom_Stage=R<std::uint64_t>(base,0x2c3e630);Geom_VertexDecl=R<int>(base,0x2c3e63c);Geom_PrimState=R<int>(base,0x2c41500);
        Geom_Material=R<const Byte*>(base,0x2c425e0);Geom_TechniqueType=R<Byte>(base,0x2c425e8);Geom_Technique=R<void*>(base,0x2c425f0);
        if(op){std::memcpy(cap.verts,base+0x283e660+(Geom_VertexCount-4)*32,128);std::memcpy(cap.idx,base+0x2a3e620+(Geom_IndexCount-6)*2,12);}
    }else{
        if(op==0)BeginSurface(material,type);
        if(op==1)StretchPic(material,x,y,w,h,s0,t0,s1,t1,c);
        if(op==2)StretchPicRotate(material,x,y,z,w,h,s0,t0,s1,t1,sn,cs,c);
        if(op==3)GlyphQuad3D(material,o,a,b,glyph,c);
        if(op){std::memcpy(cap.verts,vertices+Geom_VertexCount-4,128);std::memcpy(cap.idx,indices+Geom_IndexCount-6,12);}
    }
    cap.indexCount=Geom_IndexCount;cap.vertexCount=Geom_VertexCount;cap.stage=int(Geom_Stage);cap.spare=int(Geom_Stage>>32);cap.decl=Geom_VertexDecl;cap.prim=Geom_PrimState;cap.techType=Geom_TechniqueType;cap.material=reinterpret_cast<std::uint64_t>(Geom_Material);cap.technique=reinterpret_cast<std::uint64_t>(Geom_Technique);
    std::memcpy(out,&cap,sizeof(cap));
}
}
}
