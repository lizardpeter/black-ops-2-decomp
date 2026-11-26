#define T6_SOURCE "gfx_d3d/r_rendertarget.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "gfx_d3d/r_render_target_create.h"
#include "gfx_d3d/r_extra_cam_targets.h"
#include <string.h>
extern int store_142155DFC;
extern "C" {
extern StoreRenderTarget store_renderTargets[72];
extern StoreExtraCamSettings store_extraCamSettings;
extern unsigned store_depthTextureFormat;
}
extern "C" void Store_InitSizedTargetWithDepth(int index,int w,int h,unsigned,unsigned format,unsigned,StoreRenderTarget *target) {
    Store_ImageSetupRenderTarget(index,static_cast<unsigned short>(w<=1?1:w),static_cast<unsigned short>(h<=1?1:h),format,1,target);
    const unsigned depthFormat=store_depthTextureFormat;
    const unsigned tex[11]={static_cast<unsigned>(w),static_cast<unsigned>(h),1,1,depthFormat,1,0,0,0x40,0,0};
    ID3D11Resource *resource;
    do { T6_COM_CALL(DX(device),5,HRESULT(WINAPI *)(ID3D11Device *,const unsigned *,const void *,ID3D11Resource **),tex,nullptr,&resource); } while(store_142155DFC);
    const unsigned desc[6]={depthFormat,3,0,0,0,0};
    do { T6_COM_CALL(DX(device),10,HRESULT(WINAPI *)(ID3D11Device *,ID3D11Resource *,const unsigned *,void **),resource,desc,&target->depthView); } while(store_142155DFC);
    IUnknown_Release(resource);
}
void R_InitExtraCamRenderTargets(unsigned w,unsigned h,unsigned mw,unsigned mh,int location) {
    store_extraCamSettings.location=location;
    store_extraCamSettings.multiWidth=mw;
    store_extraCamSettings.height=h;
    store_extraCamSettings.width=w;
    store_extraCamSettings.enabled=1;
    store_extraCamSettings.multiHeight=mh;
    for(int i=0;i<6;++i) Store_InitSizedTargetWithDepth(0x1d+i,i<2?w:mw,i<2?h:mh,0,i==1?41:28,0,&store_renderTargets[19+i]);
}
void R_ShutdownExtraCamRenderTargets() {
    store_extraCamSettings.enabled=0;
    for(int i=19;i<25;++i) {
        auto &t=store_renderTargets[i];
        if(t.colorView) IUnknown_Release(t.colorView);
        if(t.depthView) IUnknown_Release(t.depthView);
        if(t.image) Image_Release(t.image);
        memset(&t,0,sizeof(t));
    }
}
#endif
