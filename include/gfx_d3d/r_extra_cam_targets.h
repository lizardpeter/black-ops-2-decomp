#pragma once
#ifdef T6_STORE
struct StoreRenderTarget;
struct StoreExtraCamSettings {
    unsigned char enabled;
    unsigned char padding[3];
    unsigned width, height, multiWidth, multiHeight;
    int location;
};
static_assert(sizeof(StoreExtraCamSettings)==24,"Store extra camera settings");
extern "C" void Store_InitSizedTargetWithDepth(int index,int width,int height,unsigned unused4,unsigned format,unsigned unused6,StoreRenderTarget *target);
void R_InitExtraCamRenderTargets(unsigned width,unsigned height,unsigned multiWidth,unsigned multiHeight,int location);
void R_ShutdownExtraCamRenderTargets();
#endif
