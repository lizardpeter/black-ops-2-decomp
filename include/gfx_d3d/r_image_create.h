#pragma once
struct GfxImage;
#ifdef T6_STORE
void Image_Create2DTexture_PC(GfxImage *, unsigned width, unsigned height, unsigned levels, unsigned flags, unsigned format, const void *data);
void Image_Create3DTexture_PC(GfxImage *, unsigned width, unsigned height, unsigned depth, unsigned levels, unsigned unused, unsigned format, const void *data);
void Image_CreateCubeTexture_PC(GfxImage *, unsigned width, unsigned levels, unsigned format, const void *data);
extern "C" void Store_ImageSetup(GfxImage *, int width, int height, int depth, unsigned flags, unsigned format, void *data);
#endif
