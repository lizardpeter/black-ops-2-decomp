#define T6_SOURCE "gfx_d3d/r_image.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_release.h"
#include "universal/dvar.h"
#include <string.h>
extern const dvar_t *store_imageCreateDvar;
extern "C" {
extern unsigned store_imageTotalMemory[2];
extern unsigned store_imageLoadedBytes;
extern StoreRenderTarget store_renderTargets[72];
extern unsigned char store_extraImageA[0x68], store_extraImageB[0x68], store_extraImageC[0x68];
extern unsigned long long store_renderTargetShutdownState;
}
namespace {
unsigned Read32(const unsigned char *p) {unsigned v; memcpy(&v,p,4); return v;}
}
// Original Store MP 14073B6B0, ZM 1407391E0, SP 1405DE7A0.
void Image_Release(GfxImage *image) {
    auto *p=reinterpret_cast<unsigned char *>(image);
    if(p[15]>1 && p[15]!=4) {
        store_imageTotalMemory[0]-=Read32(p+16);
        store_imageTotalMemory[1]-=Read32(p+20);
    }
    void *view; memcpy(&view,p,8);
    if(view) {
        IUnknown_Release(view);
        memset(p,0,8); memset(p+16,0,8);
    } else if(!p[11]) {
        // Store optimized away the assertion but retained the string conversion call.
        if(store_imageCreateDvar && store_imageCreateDvar->type!=DVAR_TYPE_BOOL)
            (void)Dvar_GetBool(store_imageCreateDvar);
    }
    store_imageLoadedBytes-=Read32(p+0x4c);
    memset(p+0x4c,0,4); p[8]=0;
}
// Original Store MP 140753660, ZM 1407511A0, SP 1405F9F80.
void sub_140753660() {
    for(auto &t:store_renderTargets) {
        if(t.colorView) IUnknown_Release(t.colorView);
        if(t.depthView) IUnknown_Release(t.depthView);
        if(t.image) Image_Release(t.image);
        memset(&t,0,sizeof(t));
    }
    Image_Release(reinterpret_cast<GfxImage *>(store_extraImageA));
    Image_Release(reinterpret_cast<GfxImage *>(store_extraImageB));
    Image_Release(reinterpret_cast<GfxImage *>(store_extraImageC));
    store_renderTargetShutdownState=0;
}
#endif
