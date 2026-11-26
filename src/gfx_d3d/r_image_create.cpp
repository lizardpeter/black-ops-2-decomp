#define T6_SOURCE "gfx_d3d/r_image.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_create.h"
#include "gfx_d3d/r_image_descriptor.h"
#include "gfx_d3d/r_image_memory.h"
#include "universal/dvar.h"

extern int store_142155DFC;
extern const dvar_t *store_imageCreateDvar;
namespace {
void Write16(GfxImage *image, unsigned offset, unsigned value) {
    auto *p = reinterpret_cast<unsigned char *>(image) + offset;
    p[0] = static_cast<unsigned char>(value); p[1] = static_cast<unsigned char>(value >> 8);
}
void SetDimensions(GfxImage *image, unsigned w, unsigned h, unsigned d) {
    Write16(image, 0x18, w); Write16(image, 0x1a, h); Write16(image, 0x1c, d);
}
void Create(GfxImage *image, const StoreImageDescriptor &desc, const void *data) {
    SetDimensions(image, desc.width, desc.height, desc.depth);
    reinterpret_cast<unsigned char *>(image)[8] = static_cast<unsigned char>(desc.type);
    ID3D11Resource *resource;
    // Original repeats based on a global flag, not the returned HRESULT.
    do {
        auto *device = DX(device);
        T6_COM_CALL(device, desc.type == 4 ? 6 : 5, HRESULT(WINAPI *)(ID3D11Device *, const unsigned *, const void *, ID3D11Resource **), desc.texture, data, &resource);
    } while (store_142155DFC);
    do {
        auto *device = DX(device);
        T6_COM_CALL(device, 7, HRESULT(WINAPI *)(ID3D11Device *, ID3D11Resource *, const unsigned *, GfxImage *), resource, desc.view, image);
    } while (store_142155DFC);
    IUnknown_Release(resource);
}
}
void Image_Create2DTexture_PC(GfxImage *image, unsigned w, unsigned h, unsigned levels, unsigned flags, unsigned fmt, const void *data) {
    StoreImageDescriptor desc;
    Store_DescribeImage2D(&desc, w, h, levels, flags, fmt, DX(multiSampleType), DX(multiSampleQuality));
    Create(image, desc, data);
}
void Image_Create3DTexture_PC(GfxImage *image, unsigned w, unsigned h, unsigned d, unsigned levels, unsigned, unsigned fmt, const void *data) {
    StoreImageDescriptor desc;
    Store_DescribeImage3D(&desc, w, h, d, levels, fmt);
    Create(image, desc, data);
}
void Image_CreateCubeTexture_PC(GfxImage *image, unsigned w, unsigned levels, unsigned fmt, const void *data) {
    StoreImageDescriptor desc;
    Store_DescribeImageCube(&desc, w, levels, fmt);
    Create(image, desc, data);
}
extern "C" void Store_ImageSetup(GfxImage *image, int w, int h, int d, unsigned flags, unsigned fmt, void *data) {
    SetDimensions(image, w, h, d);
    if (!Dvar_GetBool(store_imageCreateDvar)) return;
    const unsigned levels = (flags >> 1) & 1;
    if (flags & 4) Image_CreateCubeTexture_PC(image, w & 65535, levels, fmt, data);
    else if (flags & 8) Image_Create3DTexture_PC(image, w & 65535, h & 65535, d & 65535, levels, 0, fmt, data);
    else Image_Create2DTexture_PC(image, w & 65535, h & 65535, levels, flags, fmt, data);
    Store_ImageTrackMemory(image, flags, fmt, w, h, d);
}
#endif
