#define T6_SOURCE "gfx_d3d/r_image.cpp"
#include "gfx_d3d/r_image_memory.h"

#ifdef T6_STORE
extern "C" unsigned Store_ImageMipBytes(unsigned format, unsigned width,
                                        unsigned height, unsigned depth)
{
    const unsigned volume = width * height * depth;
    switch (format) {
    case 10: case 11: return volume * 8;
    case 24: case 26: case 27: case 28: case 29:
    case 34: case 35: case 40: case 41: case 45: return volume * 4;
    case 55: case 85: return volume * 2;
    case 65: return volume;
    case 71: return ((width + 3) >> 2) * ((height + 3) >> 2) * depth * 8;
    case 74: case 77: case 83:
        return ((width + 3) >> 2) * ((height + 3) >> 2) * depth * 16;
    default: return 0;
    }
}
namespace {
unsigned Picmip(unsigned value, unsigned shift)
{
    // Store uses SAR with the x86 masked shift count and signed max(1,x).
    shift &= 31;
    if (value & 0x80000000u) return 1;
    value >>= shift;
    return value > 1 ? value : 1;
}
unsigned Half(unsigned value) { value >>= 1; return value > 1 ? value : 1; }
}
extern "C" void Store_ImageTrackMemory(void *image, unsigned flags, unsigned format,
                                      unsigned width, unsigned height, unsigned depth)
{
    auto *bytes = static_cast<unsigned char *>(image);
    for (unsigned platform = 0; platform < 2; ++platform) {
        unsigned w = width, h = height, d = depth;
        if (!(flags & 1)) {
            const unsigned shift = bytes[0x0c + platform];
            w = Picmip(w, shift); h = Picmip(h, shift); d = Picmip(d, shift);
        }
        unsigned total = Store_ImageMipBytes(format, w, h, d);
        if (!(flags & 2)) {
            while (w + h + d > 3) {
                w = Half(w); h = Half(h); d = Half(d);
                total += Store_ImageMipBytes(format, w, h, d);
            }
        }
        if (flags & 4) total *= 6;
        for (unsigned i = 0; i < 4; ++i)
            bytes[0x10 + platform * 4 + i] = static_cast<unsigned char>(total >> (8 * i));
    }
}
#endif
