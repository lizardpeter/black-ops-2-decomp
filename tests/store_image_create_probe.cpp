#include "gfx_d3d/r_dx.h"
#include "gfx_d3d/r_image_create.h"
#include "universal/dvar.h"
#include <cstring>
#include <cstdlib>
struct Capture {unsigned n,bad,events[16],texture[11],view[6],track[5];unsigned char image[64];};
static Capture cap;
static void *deviceSlots[8],*resourceSlots[3];static void **dev=deviceSlots,**res=resourceSlots;
ID3D11Device *device=reinterpret_cast<ID3D11Device *>(&dev);
unsigned multiSampleType,multiSampleQuality;
int store_142155DFC;
static dvar_t var;const dvar_t *store_imageCreateDvar;
static int *retry;static unsigned createLeft,viewLeft,texWords;static unsigned char image[64];static const unsigned char data[8]={};static const void *expectedData;
static void event(unsigned e){if(cap.n<16)cap.events[cap.n++]=e;else ++cap.bad;}
static int WINAPI create(void *self,const unsigned *desc,const void *initial,void **out){event(1);if(self!=&dev||initial!=expectedData)++cap.bad;memcpy(cap.texture,desc,texWords*4);*out=&res;*retry=--createLeft?1:0;return createLeft?-1:0;}
static int WINAPI view(void *self,void *resource,const unsigned *desc,void **out){event(2);if(self!=&dev||resource!=&res||out!=reinterpret_cast<void**>(image))++cap.bad;memcpy(cap.view,desc,24);*out=&res;*retry=--viewLeft?1:0;return viewLeft?-1:0;}
static unsigned WINAPI release(void *self){event(3);if(self!=&res)++cap.bad;return 1;}
bool Dvar_GetBool(const dvar_t *d){return d&&(d->type==1?d->current.enabled:atoi(d->current.string)!=0);}
extern "C" void Store_ImageTrackMemory(void *p,unsigned flags,unsigned fmt,unsigned w,unsigned h,unsigned d){event(4);if(p!=image)++cap.bad;cap.track[0]=flags;cap.track[1]=fmt;cap.track[2]=w;cap.track[3]=h;cap.track[4]=d;}
static void WINAPI track(void *p,unsigned f,unsigned fmt,unsigned w,unsigned h,unsigned d){Store_ImageTrackMemory(p,f,fmt,w,h,d);}
static int WINAPI parse(const char *s){return atoi(s);}
extern "C" void *Mock(unsigned i){deviceSlots[5]=(void*)create;deviceSlots[6]=(void*)create;deviceSlots[7]=(void*)view;resourceSlots[2]=(void*)release;switch(i){case 0:return &dev;case 1:return (void*)parse;default:return (void*)track;}}
extern "C" void CaptureCreate(void *entry,void *globals,unsigned kind,unsigned w,unsigned h,unsigned d,unsigned levels,unsigned flags,unsigned fmt,unsigned samples,unsigned quality,unsigned mode,unsigned cr,unsigned vr,unsigned useData,Capture *out){
 cap={};memset(image,0xa5,sizeof(image));multiSampleType=samples;multiSampleQuality=quality;createLeft=cr;viewLeft=vr;expectedData=useData?data:nullptr;texWords=(kind==4||(kind==0&&!(flags&4)&&(flags&8)))?9:11;var={};var.type=mode<3?1:8;if(mode<3)var.current.enabled=mode==2;else var.current.string=mode==3?"0":"-2";store_imageCreateDvar=mode?&var:nullptr;
 auto *g=static_cast<unsigned char *>(globals);retry=entry?reinterpret_cast<int*>(g+16):&store_142155DFC;*retry=0;
 auto *im=reinterpret_cast<GfxImage*>(image);
 if(entry){memcpy(g,&samples,4);memcpy(g+4,&quality,4);memcpy(g+24,&store_imageCreateDvar,8);
 if(kind==0)reinterpret_cast<void(WINAPI *)(GfxImage*,int,int,int,unsigned,unsigned,void*)>(entry)(im,w,h,d,flags,fmt,const_cast<void*>(expectedData));
 else if(kind==3)reinterpret_cast<void(WINAPI *)(GfxImage*,unsigned,unsigned,unsigned,unsigned,unsigned,const void*)>(entry)(im,w,h,levels,flags,fmt,expectedData);
 else if(kind==4)reinterpret_cast<void(WINAPI *)(GfxImage*,unsigned,unsigned,unsigned,unsigned,unsigned,unsigned,const void*)>(entry)(im,w,h,d,levels,0xdeadbeef,fmt,expectedData);
 else reinterpret_cast<void(WINAPI *)(GfxImage*,unsigned,unsigned,unsigned,const void*)>(entry)(im,w,levels,fmt,expectedData);
 }else{if(kind==0)Store_ImageSetup(im,w,h,d,flags,fmt,const_cast<void*>(expectedData));else if(kind==3)Image_Create2DTexture_PC(im,w,h,levels,flags,fmt,expectedData);else if(kind==4)Image_Create3DTexture_PC(im,w,h,d,levels,0xdeadbeef,fmt,expectedData);else Image_CreateCubeTexture_PC(im,w,levels,fmt,expectedData);}
 memcpy(cap.image,image,64);*out=cap;
}
extern "C" unsigned CaptureSize(){return sizeof(Capture);}
