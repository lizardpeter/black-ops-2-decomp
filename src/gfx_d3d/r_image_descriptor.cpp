#define T6_SOURCE "gfx_d3d/r_image.cpp"
#include "gfx_d3d/r_image_descriptor.h"
#ifdef T6_STORE
extern "C" void Store_DescribeImage2D(StoreImageDescriptor *o, unsigned w, unsigned h,
    unsigned levels, unsigned flags, unsigned fmt, unsigned samples, unsigned quality)
{
    *o = {};
    w &= 65535; h &= 65535;
    const bool ms = (flags & 0x40000) && samples > 1;
    unsigned resource = fmt, view = fmt;
    switch (fmt) {
    case 20: resource=19; view=21; break;
    case 40: resource=39; view=41; break;
    case 45: resource=44; view=46; break;
    case 27: view=28; break;
    }
    const unsigned bind = !(flags & 0x20000) ? 8 : (fmt==20 || fmt==40 || fmt==45 ? 0x48 : 0x28);
    *o = {{w,h,levels,1,resource,ms?samples:1,ms?quality:0,0,bind,0,0},
          {view,ms?6u:4u,0,0xffffffffu,0,0},11,3,w,h,1};
}
extern "C" void Store_DescribeImage3D(StoreImageDescriptor *o, unsigned w, unsigned h,
    unsigned d, unsigned levels, unsigned fmt)
{
    w &= 65535; h &= 65535; d &= 65535;
    *o = {{w,h,d,levels,fmt,2,8,0x10000,0,0,0}, {fmt,8,0,levels,0,0},9,4,w,h,d};
}
extern "C" void Store_DescribeImageCube(StoreImageDescriptor *o, unsigned w, unsigned levels, unsigned fmt)
{
    w &= 65535;
    *o = {{w,w,levels,6,fmt,1,0,0,8,0,4}, {fmt,9,0,0xffffffffu,0,0},11,5,w,w,1};
}
#endif
