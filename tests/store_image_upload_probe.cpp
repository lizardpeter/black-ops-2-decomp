#include "gfx_d3d/r_image_upload.h"
#include <cstring>
#define MS __attribute__((ms_abi))
struct Capture { StoreImageUploadPlan plan; unsigned count, bad; unsigned events[6]; };
static Capture cap;
static void *viewSlots[8], *resourceSlots[11], *contextSlots[49];
static void **view=viewSlots, **resource=resourceSlots, **context=contextSlots;
alignas(8) static unsigned char image[64];
static const unsigned char pixels[16]={};
static unsigned levels, expectedFormat, expectedMip;
static void event(unsigned value) {if(cap.count<6) cap.events[cap.count]=value; else ++cap.bad; ++cap.count;}
static void MS getResource(void *self,void **out) {event(1);if(self!=&view)++cap.bad;*out=&resource;}
static void MS getDesc(void *self,unsigned *out) {event(2);if(self!=&resource)++cap.bad;std::memset(out,0,44);out[2]=levels;}
static void MS enter(void *) {event(3);}
static void MS update(void *self,void *res,unsigned sub,void *box,const void *data,unsigned pitch,unsigned depth) {
    event(4);if(self!=&context || res!=&resource || box || data!=pixels)++cap.bad;
    cap.plan={STORE_UPLOAD_UPDATE,sub,pitch,depth};
}
static void MS leave(void *) {event(5);}
static unsigned MS release(void *self) {event(6);if(self!=&resource)++cap.bad;return 1;}
static void MS volume(void *self,unsigned fmt,unsigned mip,const void *data) {event(7);if(self!=image || fmt!=expectedFormat || mip!=expectedMip || data!=pixels)++cap.bad;cap.plan.kind=STORE_UPLOAD_VOLUME;}
extern "C" void *UploadPointer(unsigned i) {
    viewSlots[7]=reinterpret_cast<void*>(getResource);resourceSlots[10]=reinterpret_cast<void*>(getDesc);resourceSlots[2]=reinterpret_cast<void*>(release);contextSlots[48]=reinterpret_cast<void*>(update);
    switch(i){case 0:return &context;case 1:return reinterpret_cast<void*>(enter);case 2:return reinterpret_cast<void*>(leave);default:return reinterpret_cast<void*>(volume);}
}
extern "C" void CaptureUpload(void *entry,unsigned type,unsigned w,unsigned fmt,unsigned face,unsigned mip,unsigned resourceLevels,Capture *out) {
    cap={};levels=resourceLevels;expectedFormat=fmt;expectedMip=mip;std::memset(image,0,sizeof(image));void *v=&view;std::memcpy(image,&v,sizeof(v));image[8]=static_cast<unsigned char>(type);unsigned short width=static_cast<unsigned short>(w);std::memcpy(image+24,&width,2);
    reinterpret_cast<void (MS *)(void*,unsigned,unsigned,unsigned,const void*)>(entry)(image,fmt,face,mip,pixels);*out=cap;
}
