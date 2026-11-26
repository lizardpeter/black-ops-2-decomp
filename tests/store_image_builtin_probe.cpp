#include "gfx_d3d/r_image_builtin.h"
#include <cstring>
#include <cstdint>

struct GfxImage {};
struct Capture {
    unsigned setupCount, uploadCount, width, height, depth, flags, setupFormat, uploadFormat, face, mip;
    unsigned char pixels[16];
    unsigned bad;
};
static Capture capture;
static GfxImage image;
extern "C" void Store_ImageSetup(GfxImage *p, int w, int h, int d, unsigned flags, unsigned fmt, void *data)
{
    ++capture.setupCount;
    capture.width=w; capture.height=h; capture.depth=d; capture.flags=flags; capture.setupFormat=fmt;
    if (p!=&image || data || capture.uploadCount) ++capture.bad;
}
extern "C" void Store_ImageUploadData(GfxImage *p, unsigned fmt, int face, int mip, const unsigned char *pixels)
{
    ++capture.uploadCount;
    capture.uploadFormat=fmt; capture.face=face; capture.mip=mip;
    if (p!=&image || capture.setupCount!=1 || capture.width!=1 || (capture.height!=1 && capture.height!=4) || !pixels) { ++capture.bad; return; }
    std::memcpy(capture.pixels,pixels,capture.height*4);
}
extern "C" __attribute__((ms_abi)) void ReplaySetup(GfxImage *p,int w,int h,int d,unsigned f,unsigned fmt,void *data) { Store_ImageSetup(p,w,h,d,f,fmt,data); }
extern "C" __attribute__((ms_abi)) void ReplayUpload(GfxImage *p,unsigned fmt,int face,int mip,const unsigned char *pixels) { Store_ImageUploadData(p,fmt,face,mip,pixels); }
using Loader=void (*)(GfxImage *);
static Loader loaders[]={Image_LoadWhite,Image_LoadBlack,Image_LoadBlank,Image_LoadGray,Image_LoadLinearGray,Image_LoadIdentityNormalMap,Image_LoadWhiteTransparent,
#define L(N) Image_LoadLightmapIntensity<N>,
L(1) L(2) L(3) L(4) L(5) L(6) L(7) L(8) L(9) L(10) L(11) L(12) L(13) L(14) L(15) L(17) L(18) L(19) L(21) L(23) L(25) L(28) L(31)
#undef L
};
extern "C" void CaptureNative(unsigned index, unsigned rgba, Capture *out)
{
    capture={};
    if (!index) Image_Generate2D(&image,rgba,rgba>>8,rgba>>16,rgba>>24);
    else loaders[index-1](&image);
    *out=capture;
}
extern "C" void CaptureOriginal(void *entry, unsigned index, unsigned rgba, Capture *out)
{
    capture={};
    if (!index) reinterpret_cast<void (__attribute__((ms_abi)) *)(GfxImage *,unsigned char,unsigned char,unsigned char,unsigned char)>(entry)(&image,rgba,rgba>>8,rgba>>16,rgba>>24);
    else reinterpret_cast<void (__attribute__((ms_abi)) *)(GfxImage *)>(entry)(&image);
    *out=capture;
}
