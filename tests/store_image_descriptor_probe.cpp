#include "gfx_d3d/r_image_descriptor.h"
#include <cstring>
#include <cstdint>
struct Capture { StoreImageDescriptor desc; unsigned create,view,release,bad; };
static Capture cap;
static void *deviceSlots[8], *resourceSlots[3];
static void **device=deviceSlots, **resource=resourceSlots;
static unsigned char image[64];
static const unsigned char data[16]={};
static unsigned expectedWords;
#define MS __attribute__((ms_abi))
static int MS create(void *self,const unsigned *desc,const void *initial,void **out) {
    ++cap.create;
    if(self!=&device || initial!=data || cap.view || !out) ++cap.bad;
    std::memcpy(cap.desc.texture,desc,expectedWords*4); cap.desc.textureWords=expectedWords;
    *out=&resource;return 0;
}
static int MS view(void *self,void *res,const unsigned *desc,void **out) {
    ++cap.view;
    if(self!=&device || res!=&resource || out!=reinterpret_cast<void**>(image) || cap.create!=1) ++cap.bad;
    std::memcpy(cap.desc.view,desc,24); *out=&resource;return 0;
}
static unsigned MS release(void *self) {++cap.release;if(self!=&resource || cap.view!=1) ++cap.bad;return 1;}
extern "C" void *DescriptorDevice() {deviceSlots[5]=reinterpret_cast<void*>(create);deviceSlots[6]=reinterpret_cast<void*>(create);deviceSlots[7]=reinterpret_cast<void*>(view);resourceSlots[2]=reinterpret_cast<void*>(release);return &device;}
extern "C" void CaptureDescriptor(void *entry,unsigned kind,unsigned w,unsigned h,unsigned d,unsigned levels,unsigned flags,unsigned fmt,Capture *out) {
    cap={}; std::memset(image,0xA5,sizeof(image));expectedWords=kind==4?9:11;
    if(kind==3) reinterpret_cast<void (MS *)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,const void*)>(entry)(image,w,h,levels,flags,fmt,data);
    else if(kind==4) reinterpret_cast<void (MS *)(void*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,const void*)>(entry)(image,w,h,d,levels,0xdeadbeef,fmt,data);
    else reinterpret_cast<void (MS *)(void*,unsigned,unsigned,unsigned,const void*)>(entry)(image,w,levels,fmt,data);
    cap.desc.type=image[8]; unsigned short dims[3]; std::memcpy(dims,image+24,6);
    cap.desc.width=dims[0];cap.desc.height=dims[1];cap.desc.depth=dims[2];
    for(unsigned i=9;i<24;++i) if(image[i]!=0xA5) ++cap.bad;
    for(unsigned i=30;i<64;++i) if(image[i]!=0xA5) ++cap.bad;
    *out=cap;
}
