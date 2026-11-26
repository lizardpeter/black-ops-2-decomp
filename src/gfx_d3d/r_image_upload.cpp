#define T6_SOURCE "gfx_d3d/r_image.cpp"
#include "gfx_d3d/r_image_upload.h"
#ifdef T6_STORE
extern "C" void Store_PlanImageUpload(StoreImageUploadPlan *out, unsigned type, unsigned width,
    unsigned format, unsigned face, unsigned mip, unsigned levels)
{
    *out = {};
    if ((type & 255) == 4) { out->kind = STORE_UPLOAD_VOLUME; return; }
    width = (width & 65535) >> (mip & 31);
    if (!width) width = 1;
    unsigned pitch;
    switch (format) {
    case 10: case 11: case 28: case 35: case 41: pitch=width*4; break;
    case 65: pitch=width; break;
    case 71: pitch=((width+3)>>2)*8; break;
    case 74: case 77: pitch=((width+3)>>2)*16; break;
    case 85: pitch=width*2; break;
    default: return;
    }
    *out = {STORE_UPLOAD_UPDATE, face*levels+mip, pitch, 0};
}
#endif
