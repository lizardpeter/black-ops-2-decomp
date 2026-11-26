#pragma once
#ifdef T6_STORE
struct GfxImage;
struct StoreRenderTarget {
    GfxImage *image;
    void *colorView;
    void *depthView;
    unsigned short width, height;
    unsigned tail;
};
static_assert(sizeof(StoreRenderTarget)==32, "Store render target stride");
void Image_Release(GfxImage *image);
void sub_140753660(); // original name not recovered: render-target shutdown
#endif
