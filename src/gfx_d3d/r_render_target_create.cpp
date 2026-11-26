#define T6_SOURCE "gfx_d3d/r_image.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "gfx_d3d/r_image_prog.h"
#include "gfx_d3d/r_image_builtin.h"
#include "gfx_d3d/r_render_target_create.h"
#include <string.h>
extern int store_142155DFC;
extern "C" StoreRenderTarget store_renderTargets[72];
// MP 140752470, ZM 14074FFB0, SP 1405F8D90.
extern "C" void Store_ImageSetupRenderTarget(int index, unsigned width, unsigned height,
    unsigned format, int colorTarget, StoreRenderTarget *target) {
    width &= 65535; height &= 65535;
    auto *image=Image_AllocProg(index,5,0);
    target->image=image;
    const char *name;
    memcpy(&name,reinterpret_cast<const unsigned char *>(image)+0x58,sizeof(name));
    unsigned flags=0x20003;
    if(!strcmp(name,"$scene")) flags=0x60003;
    else if(!strcmp(name,"$ssao_csz") || !strcmp(name,"$dof_scene_fp16")) flags=0x30001;
    Store_ImageSetup(image,width,height,1,flags,format,nullptr);
    ID3D11View *view;
    memcpy(&view,target->image,sizeof(view));
    ID3D11Resource *resource;
    if(!colorTarget) {
        const unsigned desc[6]={format,3,0,0,0,0};
        ID3D11View_GetResource(view,&resource);
        T6_COM_CALL(DX(device),10,HRESULT(WINAPI *)(ID3D11Device *,ID3D11Resource *,const unsigned *,void **),resource,desc,&target->depthView);
    } else {
        const unsigned dim=(target==&store_renderTargets[4] && DX(multiSampleType)>1)?6:4;
        const unsigned desc[6]={format==27?28u:format,dim,0,0,0,0};
        ID3D11View_GetResource(view,&resource);
        do {
            T6_COM_CALL(DX(device),9,HRESULT(WINAPI *)(ID3D11Device *,ID3D11Resource *,const unsigned *,void **),resource,desc,&target->colorView);
        } while(store_142155DFC);
    }
    IUnknown_Release(resource);
    target->height=static_cast<unsigned short>(height);
    target->width=static_cast<unsigned short>(width);
}
#endif
