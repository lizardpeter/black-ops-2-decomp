#define T6_SOURCE "gfx_d3d/r_image.cpp"
#ifdef T6_STORE
#include "gfx_d3d/r_image_prog.h"
#include <string.h>
extern "C" {
extern unsigned char store_imageProgs[];
extern const char *store_imageProgNames[];
extern GfxImage *store_imageHashTable[2048];
}
// Store MP 14073B740, ZM 140739270, SP 1405DE830.
GfxImage *Image_AllocProg(int index, unsigned char category, unsigned char semantic) {
    auto *p=store_imageProgs+static_cast<__PTRDIFF_TYPE__>(index)*0x68;
    const char *name=store_imageProgNames[index];
    p[9]=semantic; memcpy(p+0x58,&name,sizeof(name)); p[10]=category; p[15]=0;
    unsigned hash=0;
    for(const char *c=name;*c;++c) hash=hash*33u^static_cast<unsigned>(static_cast<signed char>(*c));
    unsigned slot=hash&2047u, attempts=0;
    while(store_imageHashTable[slot]) {
        slot=(slot+1)&2047u;
        if(++attempts>2048u) break;
    }
    auto *image=reinterpret_cast<GfxImage *>(p);
    store_imageHashTable[slot]=image;
    return image;
}
#endif
