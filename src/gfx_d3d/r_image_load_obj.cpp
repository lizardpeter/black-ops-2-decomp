#define T6_SOURCE "gfx_d3d/r_image_load_obj.cpp"
#include "gfx_d3d/r_image_builtin.h"

#ifdef T6_STORE
namespace {
constexpr unsigned rgba8Unorm = 28;
void GenerateBuiltin(GfxImage *image, int height, const unsigned char *pixels)
{
    Store_ImageSetup(image, 1, height, 1, 3, rgba8Unorm, nullptr);
    Store_ImageUploadData(image, rgba8Unorm, 0, 0, pixels);
}
}

void Image_Generate2D(GfxImage *image, unsigned char r, unsigned char g,
                      unsigned char b, unsigned char a)
{
    const unsigned char pixels[4] = {r, g, b, a};
    GenerateBuiltin(image, 1, pixels);
}

void Image_LoadWhite(GfxImage *image) { Image_Generate2D(image, 255, 255, 255, 255); }
void Image_LoadBlack(GfxImage *image) { Image_Generate2D(image, 0, 0, 0, 255); }
void Image_LoadBlank(GfxImage *image) { Image_Generate2D(image, 0, 0, 0, 0); }
void Image_LoadGray(GfxImage *image) { Image_Generate2D(image, 128, 128, 128, 128); }
void Image_LoadLinearGray(GfxImage *image) { Image_Generate2D(image, 180, 180, 180, 128); }
void Image_LoadIdentityNormalMap(GfxImage *image) { Image_Generate2D(image, 128, 128, 255, 128); }
void Image_LoadWhiteTransparent(GfxImage *image) { Image_Generate2D(image, 255, 255, 255, 0); }

template<int N> void Image_LoadLightmapIntensity(GfxImage *image)
{
    static_assert(N >= 1 && N <= 31);
    const unsigned char pixels[16] = {255, 255, 255, static_cast<unsigned char>(255 / N)};
    GenerateBuiltin(image, 4, pixels);
}

// One specialization per distinct original body; several registrations
// alias the same body after constant folding/identical code folding.
#define T6_LIGHTMAP(N) template void Image_LoadLightmapIntensity<N>(GfxImage *);
T6_LIGHTMAP(1) T6_LIGHTMAP(2) T6_LIGHTMAP(3) T6_LIGHTMAP(4)
T6_LIGHTMAP(5) T6_LIGHTMAP(6) T6_LIGHTMAP(7) T6_LIGHTMAP(8)
T6_LIGHTMAP(9) T6_LIGHTMAP(10) T6_LIGHTMAP(11) T6_LIGHTMAP(12)
T6_LIGHTMAP(13) T6_LIGHTMAP(14) T6_LIGHTMAP(15) T6_LIGHTMAP(17)
T6_LIGHTMAP(18) T6_LIGHTMAP(19) T6_LIGHTMAP(21) T6_LIGHTMAP(23)
T6_LIGHTMAP(25) T6_LIGHTMAP(28) T6_LIGHTMAP(31)
#undef T6_LIGHTMAP
#endif
