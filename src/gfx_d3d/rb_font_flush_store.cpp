// Original Store MP RB_EndTessSurface, 14073D680.
#define T6_SOURCE "gfx_d3d/rb_backend.cpp"
#if defined(T6_STORE) && defined(T6_STORE_FONT_REPLAY)
#include <cstdint>
#include <cstring>
namespace store_font_state{
void GetViewport(const unsigned char*,int*);
void SetViewportConstants(unsigned char*,const int*);
void SetVertexBuffer(unsigned char*,const void*,int,int);
}
namespace store_font_gpu{
int UploadIndices(unsigned char*,const void*,int);
int UploadVertices(unsigned char*,const void*,int);
void SetPass(unsigned char**,std::uint32_t);
void BindVertexShader(unsigned char*);
void SetCodeConstant(unsigned char**,const unsigned char*);
void SetStableArgs(unsigned char**);
void SetMaterialArgs(unsigned char**,int,const unsigned char*);
void DrawIndexed(unsigned char*,const int*);
}
namespace store_font_geom {
struct Vertex;
extern "C" {
extern Vertex*Geom_Vertices;
extern std::uint16_t*Geom_Indices;
extern int Geom_IndexCount,Geom_VertexCount;
extern void*Geom_Technique;
}
}
namespace store_font_flush{
using Byte=unsigned char;using U16=std::uint16_t;
template<class T>static T R(const void*p,unsigned n){T v;std::memcpy(&v,static_cast<const Byte*>(p)+n,sizeof(v));return v;}
extern "C"{
extern Byte*Flush_Source,*Flush_State;
extern Byte*Flush_RenderTargets,*Flush_DynamicBuffers;
extern Byte Flush_Target,Flush_ViewportDirty,Flush_ScissorEnabled;
extern int Flush_ViewportCache[4],Flush_ScissorCache[4],Flush_Scissor[4],Flush_ScissorActive;
extern float Flush_MinDepth,Flush_MaxDepth;
void Flush_SetViewport(void*,unsigned,const float*);
void Flush_SetScissor(void*,unsigned,const int*);
}
static bool Equal4(const int*a,const int*b){return a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3];}
static void ClipAxis(int&x,int&width,int limit){
    if(x<0){width+=x;x=0;if(width<1)width=0;}
    if(x>limit){x=limit;width=0;}
    if(x+width>limit)width=limit-x;
}
void EndTessSurface(){
    Byte*source=Flush_Source,*state=Flush_State;
    void*context=R<void*>(state,0xd0);
    if(Flush_ViewportDirty){
        int vp[4];store_font_state::GetViewport(source,vp);
        if(!Equal4(vp,Flush_ViewportCache)){
            std::memcpy(Flush_ViewportCache,vp,16);
            const float viewport[6]={float(vp[0]),float(vp[1]),float(vp[2]),float(vp[3]),Flush_MinDepth,Flush_MaxDepth};
            Flush_SetViewport(context,1,viewport);
        }
        store_font_state::SetViewportConstants(source,vp);
        int w=R<U16>(Flush_RenderTargets,32*Flush_Target+0x18),h=R<U16>(Flush_RenderTargets,32*Flush_Target+0x1a);
        if(!Flush_ScissorEnabled){
            if(Flush_ScissorActive){
                Flush_ScissorActive=0;
                const int full[4]={0,0,w,h},max[4]={0,0,0x2000,0x2000};
                Flush_SetScissor(context,1,full);Flush_SetScissor(context,1,max);
            }
        }else{
            int sc[4];std::memcpy(sc,Flush_Scissor,16);
            ClipAxis(sc[0],sc[2],w);ClipAxis(sc[1],sc[3],h);
            if(!Flush_ScissorActive||!Equal4(sc,Flush_ScissorCache)){
                std::memcpy(Flush_ScissorCache,sc,16);Flush_ScissorActive=1;
                sc[2]+=sc[0];sc[3]+=sc[1];Flush_SetScissor(context,1,sc);
            }
        }
    }
    int args[3]={store_font_geom::Geom_VertexCount,store_font_geom::Geom_IndexCount/3,0};
    args[2]=store_font_gpu::UploadIndices(state,store_font_geom::Geom_Indices,store_font_geom::Geom_IndexCount);
    const Byte*technique=static_cast<const Byte*>(store_font_geom::Geom_Technique);
    const Byte*stream=R<const Byte*>(Flush_DynamicBuffers,0x4be9d8);
    int offset=store_font_gpu::UploadVertices(state,store_font_geom::Geom_Vertices,store_font_geom::Geom_VertexCount);
    for(unsigned i=0;i<R<U16>(technique,0xa);++i){
        Byte*pair[2]={source,state};store_font_gpu::SetPass(pair,i);
        store_font_gpu::BindVertexShader(state);
        const Byte*pass=R<const Byte*>(state,0x11d8);
        if(pass[0x1a])store_font_gpu::SetMaterialArgs(pair,pass[0x1a],R<const Byte*>(pass,0x20)+(pass[0x18]+pass[0x19])*16);
        store_font_state::SetVertexBuffer(state+0xd0,R<const void*>(stream,8),offset,0x20);
        store_font_gpu::SetStableArgs(pair);
        pass=R<const Byte*>(state,0x11d8);
        unsigned n=pass[0x18];const Byte*arg=R<const Byte*>(pass,0x20);
        while(n&&R<U16>(arg,0)==3){store_font_gpu::SetCodeConstant(pair,arg);arg+=16;--n;}
        store_font_gpu::DrawIndexed(state+0xd0,args);
    }
    store_font_geom::Geom_IndexCount=0;store_font_geom::Geom_VertexCount=0;
}
}
#endif
