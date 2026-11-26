#include "gfx_d3d/r_image_memory.h"
extern "C" unsigned ReplayMip(void *entry, unsigned f, unsigned w, unsigned h, unsigned d)
{
    return reinterpret_cast<unsigned (__attribute__((ms_abi)) *)(unsigned,unsigned,unsigned,unsigned)>(entry)(f,w,h,d);
}
extern "C" void ReplayTrack(void *entry, void *image, unsigned flags, unsigned f, unsigned w, unsigned h, unsigned d)
{
    reinterpret_cast<void (__attribute__((ms_abi)) *)(void *,unsigned,unsigned,unsigned,unsigned,unsigned)>(entry)(image,flags,f,w,h,d);
}
