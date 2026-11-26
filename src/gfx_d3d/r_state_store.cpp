#define T6_SOURCE "gfx_d3d/r_state.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "gfx_d3d/r_state_store.h"
#include <string.h>
extern "C" {extern StoreRenderTarget store_renderTargets[72];extern unsigned store_shaderImageSlotCount;}
namespace {
template<class T> T Read(const void *p,unsigned off) {T v;memcpy(&v,static_cast<const unsigned char *>(p)+off,sizeof(v));return v;}
template<class T> void Write(void *p,unsigned off,T v){memcpy(static_cast<unsigned char *>(p)+off,&v,sizeof(v));}
}
void Store_ResetWorldMatrix(GfxCmdBufSourceState *source){
    const unsigned offsets[]={0x1aa6,0x1aa8,0x1aaa,0x1aae,0x1aac,0x1ab0,0x1ab4};
    for(unsigned off:offsets)Write(source,off,static_cast<unsigned short>(Read<unsigned short>(source,off)+1));
    Write(source,0x1a66,Read<unsigned short>(source,0x1aa6));
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};memcpy(source,identity,sizeof(identity));
    for(unsigned i=0;i<3;++i)Write(source,0x30+4*i,identity[12+i]-Read<float>(source,0x1ac0+4*i));
}
void Store_Set3DView(GfxCmdBufSourceState *source){
    if(Read<unsigned>(source,0x1b10)==1)return;
    const auto *input=Read<const unsigned char *>(source,0x1ae0);
    Write(source,0x1b10,1u);
    // Original copies forward in 16-byte units.
    for(unsigned i=0;i<0x150;i+=16){unsigned char v[16];memcpy(v,input+i,16);memcpy(reinterpret_cast<unsigned char *>(source)+0x1730+i,v,16);}
    // UCOMISS followed by JE treats unordered as equal too.
    const float flag=Read<float>(source,0x183c);
    if(flag==0.0f || flag!=flag){Write(source,0x1ac0,0ull);Write(source,0x1ac8,0u);}
    else for(unsigned i=0;i<3;++i)Write(source,0x1ac0+4*i,Read<unsigned>(source,0x1830+4*i));
    Write(source,0x1acc,0x3f800000u);Store_ResetWorldMatrix(source);
}
void R_SetRenderTarget(const StoreCmdBufContext *context,unsigned char id){
    auto *s=context->state;auto old=Read<unsigned char>(s,0x5404);if(old==id)return;
    auto &target=store_renderTargets[id];auto *dev=Read<ID3D11DeviceContext *>(s,0xd0);
    if(target.image){const unsigned count=store_shaderImageSlotCount;for(unsigned i=0;i<count;++i)if(Read<GfxImage *>(s,0x50+8*i)==target.image){Write<GfxImage *>(s,0x50+8*i,nullptr);void *nullView=nullptr;auto *current=Read<ID3D11DeviceContext *>(s,0xd0);T6_COM_CALL(current,8,void(WINAPI *)(ID3D11DeviceContext *,unsigned,unsigned,void *const *),i,1,&nullView);}}
    old=Read<unsigned char>(s,0x5404);dev=Read<ID3D11DeviceContext *>(s,0xd0);
    const auto &previous=store_renderTargets[old];const bool colorChanged=previous.colorView!=target.colorView;
    if(colorChanged){Write(s,0x53e0,0ull);Write(s,0x53e8,static_cast<unsigned>(target.width));Write(s,0x53ec,static_cast<unsigned>(target.height));Write(s,0x11e4,0xffffffffu);Write(s,0x11e8,0x3f800000ull);}
    if(colorChanged || previous.depthView!=target.depthView){
        T6_COM_CALL(dev,33,void(WINAPI *)(ID3D11DeviceContext *,unsigned,void *const *,void *),1,&target.colorView,target.depthView);
        const float vp[]={static_cast<float>(Read<int>(s,0x53e0)),static_cast<float>(Read<int>(s,0x53e4)),static_cast<float>(Read<int>(s,0x53e8)),static_cast<float>(Read<int>(s,0x53ec)),Read<float>(s,0x11ec),Read<float>(s,0x11e8)};
        T6_COM_CALL(dev,44,void(WINAPI *)(ID3D11DeviceContext *,unsigned,const float *),1,vp);
    }
    Write(s,0x5404,id);Write(context->source,0x1b10,0u);Write<unsigned char>(context->source,0x1b5c,1);
}
void Store_ClearScreen(GfxCmdBufState *state,unsigned char mask,const float *color,float depth,unsigned char stencil){
    const auto id=Read<unsigned char>(state,0x5404);auto *dev=Read<ID3D11DeviceContext *>(state,0xd0);
    if(store_renderTargets[id].colorView && (mask&4))T6_COM_CALL(dev,50,void(WINAPI *)(ID3D11DeviceContext *,void *,const float *),store_renderTargets[id].colorView,color);
    if(mask&3)T6_COM_CALL(dev,53,void(WINAPI *)(ID3D11DeviceContext *,void *,unsigned,float,unsigned char),store_renderTargets[Read<unsigned char>(state,0x5404)].depthView,mask&3,depth,stencil);
}
#endif
