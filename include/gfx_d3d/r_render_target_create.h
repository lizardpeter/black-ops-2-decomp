#pragma once
#ifdef T6_STORE
struct StoreRenderTarget;
// Store merged the reference setup and render-target view creation.
extern "C" void Store_ImageSetupRenderTarget(int index, unsigned width, unsigned height,
    unsigned format, int colorTarget, StoreRenderTarget *target);
#endif
