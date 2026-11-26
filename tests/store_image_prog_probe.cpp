#include "gfx_d3d/r_image_prog.h"
#include <cstring>
extern "C" {
unsigned char store_imageProgs[16*104];
const char *store_imageProgNames[16];
GfxImage *store_imageHashTable[2048];
}
static const char *names[]={"", "$scene", "$ssao_csz", "$dof_scene_fp16", "Abc", "abc", "\xff\x80x", "a long render target name"};
struct Capture {unsigned bad,slot;unsigned char image[16*104];unsigned table[2048];};
extern "C" void RunProg(void *entry,void *globals,unsigned seed,unsigned index,unsigned category,unsigned semantic,unsigned fill,Capture *out){
    auto *g=static_cast<unsigned char *>(globals);
    auto *im=entry?g:store_imageProgs;
    auto **nm=entry?reinterpret_cast<const char **>(g+0x800):store_imageProgNames;
    auto **tab=entry?reinterpret_cast<GfxImage **>(g+0x1000):store_imageHashTable;
    Capture c={};auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
    for(unsigned i=0;i<sizeof(store_imageProgs);++i)im[i]=next()>>24;
    for(unsigned i=0;i<16;++i)nm[i]=names[next()%8];
    for(unsigned i=0;i<2048;++i)tab[i]=(fill==100 || next()%100<fill)?reinterpret_cast<GfxImage *>(static_cast<__UINTPTR_TYPE__>(i+1)):nullptr;
    auto *p=entry?reinterpret_cast<GfxImage * (__attribute__((ms_abi)) *)(int,unsigned char,unsigned char)>(entry)(index,category,semantic):Image_AllocProg(index,category,semantic);
    if(reinterpret_cast<unsigned char *>(p)!=im+index*104)++c.bad;
    memcpy(c.image,im,sizeof(c.image));unsigned hits=0;
    for(unsigned i=0;i<2048;++i){if(tab[i]==p){++hits;c.slot=i;c.table[i]=0xffffffffu;}else c.table[i]=static_cast<unsigned>(reinterpret_cast<__UINTPTR_TYPE__>(tab[i]));}
    if(hits!=1)++c.bad;
    *out=c;
}
extern "C" unsigned ProgCaptureSize(){return sizeof(Capture);}
