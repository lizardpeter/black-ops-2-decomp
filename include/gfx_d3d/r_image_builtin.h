#pragma once

struct GfxImage;

// Store LTCG retained a specialized 1x1 RGBA8 generator rather than the
// reference build's general Image_Generate2D prototype.
#ifdef T6_STORE
void Image_Generate2D(GfxImage *, unsigned char r, unsigned char g,
                      unsigned char b, unsigned char a);
void Image_LoadWhite(GfxImage *);
void Image_LoadBlack(GfxImage *);
void Image_LoadBlank(GfxImage *);
void Image_LoadGray(GfxImage *);
void Image_LoadLinearGray(GfxImage *);
void Image_LoadIdentityNormalMap(GfxImage *);
void Image_LoadWhiteTransparent(GfxImage *);
template<int N> void Image_LoadLightmapIntensity(GfxImage *);

// Store-specialized callee ABI, not the legacy source prototypes. Names
// are provisional; setup and upload bodies are reconstructed.
extern "C" void Store_ImageSetup(GfxImage *, int width, int height, int depth,
                                 unsigned flags, unsigned format, void *data);
extern "C" void Store_ImageUploadData(GfxImage *, unsigned format, int face,
                                      int mip, const unsigned char *pixels);
extern "C" void Store_ImageUploadVolume(GfxImage *, unsigned format, unsigned mip,
                                        const unsigned char *pixels);
#endif
